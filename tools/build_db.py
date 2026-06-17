#!/usr/bin/env python3
"""
Tguide Database Tool — developer utilities

Primary purpose: generate signed_manifest.json for the bundled seed database.
Developer can also use init/validate/dump for interactive data entry via
data_adder.py and quick DB inspections.

The seed database (data/database/tguide.db) is a pre-built SQLite file
created by the developer using data_adder.py, NOT generated from YAML
during build. YAML/JSON files in tools/data/ are now user-customisation
examples read at runtime by the C++ application.

Commands:
  init       Create empty database with all tables
  validate   Validate existing DB against schema
  manifest   Generate signed_manifest.json for a DB
  dump       Dump database contents for inspection

Examples:
  python3 tools/build_db.py init
  python3 tools/build_db.py validate
  python3 tools/build_db.py manifest
  python3 tools/build_db.py dump
"""

import argparse
import os
import sys
import textwrap

# ──────────────────────────────────────────────
# Color support (optional, try/except for cross-platform)
# ──────────────────────────────────────────────

class Colors:
    """ANSI color constants — disabled automatically if unsupported."""
    GREEN = ""
    RED = ""
    YELLOW = ""
    CYAN = ""
    BOLD = ""
    RESET = ""

    @classmethod
    def init(cls):
        """Enable colors if stdout supports it."""
        if not sys.stdout.isatty():
            return
        try:
            # Windows
            if os.name == "nt":
                import ctypes
                kernel32 = ctypes.windll.kernel32
                kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass
        # Check if terminal likely supports ANSI
        term = os.environ.get("TERM", "")
        if os.name == "posix" and (term or sys.platform != "win32"):
            cls.GREEN = "\033[92m"
            cls.RED = "\033[91m"
            cls.YELLOW = "\033[93m"
            cls.CYAN = "\033[96m"
            cls.BOLD = "\033[1m"
            cls.RESET = "\033[0m"


def _ok(msg):
    print(f"{Colors.GREEN}[+] {msg}{Colors.RESET}")


def _warn(msg):
    print(f"{Colors.YELLOW}[!] {msg}{Colors.RESET}")


def _err(msg):
    print(f"{Colors.RED}[-] {msg}{Colors.RESET}")


def _info(msg):
    print(f"{Colors.CYAN}[*] {msg}{Colors.RESET}")


# ──────────────────────────────────────────────
# Default paths
# ──────────────────────────────────────────────

DEFAULT_DB_PATH = "data/database/tguide.db"
DEFAULT_MANIFEST_PATH = "data/signed_manifest.json"
DEFAULT_VERSION = "1.0.0"
DEFAULT_DB_URL = (
    "https://github.com/voidoxin/Tguide/releases/latest/download/tguide.db"
)


# ──────────────────────────────────────────────
# CLI subcommand implementations
# ──────────────────────────────────────────────

def cmd_init(args):
    """Initialize an empty database — create all 7 tables."""
    from schema import create_tables

    db_path = args.db
    os.makedirs(os.path.dirname(db_path) or ".", exist_ok=True)

    _info(f"Initializing database: {db_path}")

    import sqlite3
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA foreign_keys = ON;")
    cursor = conn.cursor()

    try:
        create_tables(cursor)
        conn.commit()
        _ok("All 7 tables created successfully.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"SQLite error: {e}")
        return 1
    finally:
        conn.close()

    # Verify
    from db_builder import validate_database
    issues = validate_database(db_path)
    if issues:
        _warn("Validation issues:")
        for issue in issues:
            print(f"    {issue}")
    else:
        _ok("Database validation passed.")
    return 0


def cmd_validate(args):
    """Validate existing database schema."""
    from db_builder import validate_database

    db_path = args.db
    import sqlite3

    _info(f"Validating database: {db_path}")

    try:
        issues = validate_database(db_path)
    except (OSError, sqlite3.Error) as e:
        _err(str(e))
        return 1

    if not issues:
        _ok("Database is valid — all tables and columns present.")
        # Print row counts
        import sqlite3
        from schema import get_all_tables
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        for tdef in get_all_tables():
            cursor.execute(f"SELECT COUNT(*) FROM {tdef['name']}")
            count = cursor.fetchone()[0]
            if count > 0:
                print(f"    {tdef['name']}: {count} rows")
            else:
                _warn(f"    {tdef['name']}: (empty)")
        conn.close()
        return 0
    else:
        _err("Validation found issues:")
        for issue in issues:
            print(f"    {issue}")
        return 1


def cmd_manifest(args):
    """Generate signed_manifest.json for a database."""
    from manifest import generate_manifest

    db_path = args.db
    version = args.version
    manifest_path = args.manifest

    _info(f"Generating manifest for: {db_path}")
    _info(f"Version: {version}")

    if generate_manifest(db_path, version, manifest_path, args.db_url):
        return 0
    return 1


def cmd_dump(args):
    """Dump database contents for inspection."""
    from db_builder import dump_database

    db_path = args.db
    import sqlite3

    try:
        output = dump_database(db_path)
    except (OSError, sqlite3.Error) as e:
        _err(str(e))
        return 1

    print(output)
    return 0


# ──────────────────────────────────────────────
# Argument parser
# ──────────────────────────────────────────────

def build_parser():
    parser = argparse.ArgumentParser(
        prog="build_db.py",
        description="Tguide Database Builder — replaces data_adder.cpp",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              %(prog)s init
              %(prog)s validate
              %(prog)s manifest
              %(prog)s dump
        """),
    )

    # Global options
    parser.add_argument(
        "--db",
        default=DEFAULT_DB_PATH,
        help=f"Path to output/input database (default: {DEFAULT_DB_PATH})",
    )

    subparsers = parser.add_subparsers(
        dest="command",
        title="Commands",
        description="Valid subcommands",
    )

    # init
    subparsers.add_parser("init", help="Create empty database with all tables")

    # validate
    subparsers.add_parser("validate", help="Validate existing DB against schema")

    # manifest
    manifest_parser = subparsers.add_parser("manifest", help="Generate signed_manifest.json for a DB")
    manifest_parser.add_argument(
        "--manifest",
        default=DEFAULT_MANIFEST_PATH,
        help=f"Path to output manifest (default: {DEFAULT_MANIFEST_PATH})",
    )
    manifest_parser.add_argument(
        "--version",
        default=DEFAULT_VERSION,
        help=f"Database version string (default: {DEFAULT_VERSION})",
    )
    manifest_parser.add_argument(
        "--db-url",
        default=DEFAULT_DB_URL,
        help=f"Download URL for the database (default: {DEFAULT_DB_URL})",
    )

    # dump
    subparsers.add_parser("dump", help="Dump database contents for inspection")

    return parser


# ──────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────

def main():
    Colors.init()

    parser = build_parser()
    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        return 1

    # Route to command handler
    handlers = {
        "init": cmd_init,
        "validate": cmd_validate,
        "manifest": cmd_manifest,
        "dump": cmd_dump,
    }

    handler = handlers[args.command]
    return handler(args)


if __name__ == "__main__":
    sys.exit(main())
