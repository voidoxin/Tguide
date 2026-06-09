#!/usr/bin/env python3
"""
Tguide Data Adder — Interactive & CLI tool for managing tguide.db

Rebuilds the functionality of the old data_adder.cpp in pure Python.

Two modes:
  1. Interactive menu mode (run with no arguments)
  2. CLI command mode (add / list / delete / manifest)

Usage:
  # Interactive menu
  python3 tools/data_adder.py

  # CLI command examples
  python3 tools/data_adder.py list tools
  python3 tools/data_adder.py add tool --name nmap --category "Network Scanning" ...
  python3 tools/data_adder.py delete vulnerability --id 3
  python3 tools/data_adder.py manifest
"""

import argparse
import hashlib
import json
import datetime
import os
import shutil
import sqlite3
import tempfile
import sys
import textwrap
from schema import create_tables


# Table name whitelist — prevents SQL injection via f-string table names
ALLOWED_TABLES = frozenset({
    "vulnerabilities", "options", "modules", "tools",
    "tool_flags", "templates", "categories"
})

def _assert_table(table):
    """Validate table name against whitelist."""
    if table not in ALLOWED_TABLES:
        raise ValueError(f"Forbidden table name: {table!r} (allowed: {sorted(ALLOWED_TABLES)})")


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

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_SCRIPT_DIR)  # tools/ -> project root
DEFAULT_DB_PATH = os.path.join(_PROJECT_ROOT, "data/database/tguide.db")
DEFAULT_MANIFEST_PATH = os.path.join(_PROJECT_ROOT, "data/signed_manifest.json")
DEFAULT_VERSION = "1.0.0"
DEFAULT_DB_URL = (
    "https://github.com/voidoxin/Tguide/releases/latest/download/tguide.db"
)


# ──────────────────────────────────────────────
# Database helpers
# ──────────────────────────────────────────────

def get_connection(db_path):
    """Open a connection, enable FK enforcement, return (conn, cursor)."""
    try:
        conn = sqlite3.connect(db_path)
        conn.execute("PRAGMA foreign_keys = ON;")
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        return conn, cursor
    except sqlite3.Error as e:
        _err(f"Failed to open database '{db_path}': {e}")
        sys.exit(1)


def close_connection(conn):
    """Close the database connection safely."""
    try:
        conn.close()
    except Exception:
        pass


# ──────────────────────────────────────────────
# Input helpers
# ──────────────────────────────────────────────

def prompt(msg, default=None, required=False):
    """Prompt user for input with optional default and validation."""
    if default is not None:
        label = f"{msg} [{default}]: "
    else:
        label = f"{msg}: "
    while True:
        try:
            val = input(label).strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return None
        if not val and default is not None:
            return default
        if not val and required:
            _warn("This field is required.")
            continue
        return val


def prompt_int(msg, default=None, required=False):
    """Prompt for an integer value."""
    while True:
        val = prompt(msg, default=default, required=required)
        if val is None:
            return None
        try:
            return int(val)
        except ValueError:
            _warn("Please enter a valid integer.")


def prompt_bool(msg, default=None):
    """Prompt for a boolean (y/n) value."""
    while True:
        val = prompt(msg, default=default)
        if val is None:
            return None
        val = val.strip().lower()
        if val in ("y", "yes", "true", "1"):
            return True
        if val in ("n", "no", "false", "0"):
            return False
        _warn("Please enter 'y' or 'n'.")


def confirm(msg="Are you sure?"):
    """Ask for confirmation."""
    val = prompt(f"{msg} (y/n)", required=True)
    return val is not None and val.lower() in ("y", "yes", "true", "1")


# ──────────────────────────────────────────────
# Display helpers
# ──────────────────────────────────────────────

def print_separator(char="=", width=60):
    print(char * width)


def print_header(title):
    print()
    print_separator()
    print(f"  {title}")
    print_separator()


# ──────────────────────────────────────────────
# CLI: List commands
# ──────────────────────────────────────────────

def cmd_list_tools(db_path):
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name, category, short_desc FROM tools ORDER BY id")
        rows = cur.fetchall()
        if not rows:
            _warn("No tools found.")
            return
        print()
        print_separator()
        print(f"  Tools ({len(rows)} total)")
        print_separator()
        for r in rows:
            print(f"  ID: {r['id']}")
            print(f"  Name:       {r['name']}")
            print(f"  Category:   {r['category']}")
            print(f"  Short Desc: {r['short_desc']}")
            print()
    finally:
        close_connection(conn)


def cmd_list_vulnerabilities(db_path):
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name, severity, platform, service FROM vulnerabilities ORDER BY id")
        rows = cur.fetchall()
        if not rows:
            _warn("No vulnerabilities found.")
            return
        print()
        print_separator()
        print(f"  Vulnerabilities ({len(rows)} total)")
        print_separator()
        for r in rows:
            print(f"  ID: {r['id']}")
            print(f"  Name:     {r['name']}")
            print(f"  Severity: {r['severity']}")
            print(f"  Platform: {r['platform']}")
            print(f"  Service:  {r['service']}")
            print()
    finally:
        close_connection(conn)


def cmd_list_modules(db_path):
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name, platform, type, description FROM modules ORDER BY id")
        rows = cur.fetchall()
        if not rows:
            _warn("No modules found.")
            return
        print()
        print_separator()
        print(f"  Modules ({len(rows)} total)")
        print_separator()
        for r in rows:
            print(f"  ID: {r['id']}")
            print(f"  Name:        {r['name']}")
            print(f"  Platform:    {r['platform']}")
            print(f"  Type:        {r['type']}")
            print(f"  Description: {r['description']}")
            print()
    finally:
        close_connection(conn)


def cmd_list_categories(db_path):
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name, display_order, description FROM categories ORDER BY id")
        rows = cur.fetchall()
        if not rows:
            _warn("No categories found.")
            return
        print()
        print_separator()
        print(f"  Categories ({len(rows)} total)")
        print_separator()
        for r in rows:
            print(f"  ID:             {r['id']}")
            print(f"  Name:           {r['name']}")
            print(f"  Display Order:  {r['display_order']}")
            print(f"  Description:    {r['description']}")
            print()
    finally:
        close_connection(conn)


# ──────────────────────────────────────────────
# CLI: Add commands
# ──────────────────────────────────────────────

def cmd_add_tool(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO tools (name, category, short_desc, description, flags_all) "
            "VALUES (?, ?, ?, ?, ?)",
            (args.name, args.category, args.short_desc, args.description, args.flags_all),
        )
        conn.commit()
        _ok(f"Tool '{args.name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add tool: {e}")
    finally:
        close_connection(conn)


def cmd_add_vulnerability(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO vulnerabilities "
            "(name, metasploit_name, discovered_date, discoverer, severity, "
            " access, platform, service, description, danger) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (args.name, args.metasploit_name, args.discovered_date, args.discoverer,
             args.severity, args.access, args.platform, args.service,
             args.description, args.danger),
        )
        conn.commit()
        _ok(f"Vulnerability '{args.name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add vulnerability: {e}")
    finally:
        close_connection(conn)


def cmd_add_module(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        mode_val = 1 if args.mode else 0
        # If mode=0, force loud=0 (matching C++ logic)
        if mode_val == 0:
            loud_val = 0
        else:
            loud_val = 1 if args.loud else 0
        cur.execute(
            "INSERT INTO modules "
            "(name, path, platform, type, description, API, mode, loud, output) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (args.name, args.path, args.platform, args.type,
             args.description, args.api, mode_val, loud_val, args.output),
        )
        conn.commit()
        _ok(f"Module '{args.name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add module: {e}")
    finally:
        close_connection(conn)


def cmd_add_flag(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        root_val = 1 if args.root else 0
        cur.execute(
            "INSERT INTO tool_flags (tool_id, name, description, loud, root, protocols) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (args.tool_id, args.name, args.description, args.loud, root_val, args.protocols),
        )
        conn.commit()
        _ok(f"Tool flag '{args.name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add tool flag: {e}")
    finally:
        close_connection(conn)


def cmd_add_template(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        root_val = 1 if args.root else 0
        flag_val = 1 if args.flag else 0
        cur.execute(
            "INSERT INTO templates (tool_id, template_name, description, root, protocols, flag) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (args.tool_id, args.template_name, args.description, root_val, args.protocols, flag_val),
        )
        conn.commit()
        _ok(f"Template '{args.template_name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add template: {e}")
    finally:
        close_connection(conn)


def cmd_add_category(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO categories (name, display_order, description) "
            "VALUES (?, ?, ?)",
            (args.name, args.display_order, args.description),
        )
        conn.commit()
        _ok(f"Category '{args.name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add category: {e}")
    finally:
        close_connection(conn)


def cmd_add_option(db_path, args):
    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO options (vuln_id, option_name, option_value) "
            "VALUES (?, ?, ?)",
            (args.vuln_id, args.option_name, args.option_value),
        )
        conn.commit()
        _ok(f"Option '{args.option_name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add option: {e}")
    finally:
        close_connection(conn)


# ──────────────────────────────────────────────
# CLI: Delete commands
# ──────────────────────────────────────────────

def cmd_delete_by_id(db_path, table, id_val, label="record"):
    _assert_table(table)
    conn, cur = get_connection(db_path)
    try:
        # Check existence
        cur.execute(f"SELECT id FROM {table} WHERE id = ?", (id_val,))
        if not cur.fetchone():
            _err(f"{label.capitalize()} with ID {id_val} not found.")
            return
        cur.execute(f"DELETE FROM {table} WHERE id = ?", (id_val,))
        conn.commit()
        _ok(f"{label.capitalize()} ID {id_val} deleted.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to delete: {e}")
    finally:
        close_connection(conn)


# ──────────────────────────────────────────────
# CLI: Manifest command
# ──────────────────────────────────────────────

def _sha256_file(filepath):
    """Compute SHA-256 hex digest of a file."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def cmd_manifest(db_path, args):
    version = args.version
    output_path = args.manifest
    db_url = args.db_url

    if not os.path.isfile(db_path):
        _err(f"Database file not found: {db_path}")
        return

    if not version:
        _err("Version string must not be empty.")
        return

    if not db_url.startswith("https://"):
        _err("db_url must start with https://")
        return

    db_hash = _sha256_file(db_path)

    manifest = {
        "version": version,
        "db_hash": db_hash,
        "db_url": db_url,
    }

    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    _ok(f"Manifest written to: {output_path}")
    print(f"    version  : {version}")
    print(f"    db_hash  : {db_hash}")
    print(f"    db_url   : {db_url}")


# ──────────────────────────────────────────────
# Interactive Mode
# ──────────────────────────────────────────────


# ──────────────────────────────────────────────
# Reset
# ──────────────────────────────────────────────

def cmd_reset(db_path, force=False):
    """Archive the current database and create a fresh empty one.

    Creates a temporary fresh DB first, then atomically swaps it in.
    This ensures the old DB is never deleted if creation of the new DB fails.

    Moves archived DB to: <db_dir>/old_data/tguide_<timestamp>.db
    Also archives any .bak file if present.

    Args:
        db_path: Path to existing SQLite database.
        force: If True, skip confirmation prompt (interactive mode already
               confirms before calling). When False and called from CLI,
               prompts for confirmation.
    """
    if not force:
        print("WARNING: This will archive the current database and create a fresh empty one.")
        print("Archived databases in old_data/ are never deleted automatically.")
        try:
            resp = input("Are you sure? [y/N] ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            return 1
        if resp != "y" and resp != "yes":
            _info("Reset cancelled.")
            return 1

    if not os.path.isfile(db_path):
        _err(f"Database not found: {db_path}")
        return 1

    db_dir = os.path.dirname(os.path.abspath(db_path))
    ts = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    db_name = os.path.basename(db_path)
    base_name, ext = os.path.splitext(db_name)

    # ── Step 1: Create fresh DB in temp location ──
    tmp_fd, tmp_path = tempfile.mkstemp(suffix=ext, prefix=f"{base_name}_fresh_")
    os.close(tmp_fd)
    try:
        conn = sqlite3.connect(tmp_path)
        conn.execute("PRAGMA foreign_keys = ON;")
        cursor = conn.cursor()
        create_tables(cursor)
        conn.commit()
        conn.close()
    except sqlite3.Error as e:
        _err(f"Failed to create fresh database: {e}")
        try:
            os.unlink(tmp_path)
        except OSError:
            pass
        return 1

    # ── Step 2: Create archive directory ──
    old_dir = os.path.join(db_dir, "old_data")
    try:
        os.makedirs(old_dir, exist_ok=True)
    except OSError as e:
        _err(f"Failed to create archive directory '{old_dir}': {e}")
        os.unlink(tmp_path)
        return 1

    archive_name = os.path.join(old_dir, f"{base_name}_{ts}{ext}")
    try:
        shutil.copy2(db_path, archive_name)
        _ok(f"Database archived to: {archive_name}")
    except OSError as e:
        _err(f"Failed to archive database: {e}")
        os.unlink(tmp_path)
        return 1

    # Also archive any .bak file
    bak_path = db_path + ".bak"
    if os.path.isfile(bak_path):
        bak_archive = os.path.join(old_dir, f"{base_name}_{ts}.bak")
        try:
            shutil.copy2(bak_path, bak_archive)
            _ok(f"Backup archived to: {bak_archive}")
        except OSError as e:
            _warn(f"Could not archive backup: {e}")

    # ── Step 3: Atomically swap old DB with fresh one ──
    try:
        os.remove(db_path)
    except OSError as e:
        _err(f"Failed to remove old database: {e}")
        try:
            shutil.move(tmp_path, db_path)
        except OSError:
            pass
        return 1

    shutil.move(tmp_path, db_path)
    _ok("Fresh database created with all 7 tables (empty).")

    # Warn about stale manifest
    if os.path.isfile(DEFAULT_MANIFEST_PATH):
        _warn("Existing signed_manifest.json is now stale — regenerate with 'manifest' command.")

    _ok("Database reset complete.")
    return 0


def interactive_reset(db_path):
    """Confirm and reset the database interactively."""
    print("\n=== Reset Database ===")
    _warn("This will archive the current database to old_data/ and create a fresh empty one.")
    _warn("Archived databases in old_data/ are never deleted automatically.")
    if not confirm("Are you sure you want to reset the database"):
        _info("Reset cancelled.")
        return
    if cmd_reset(db_path, force=True) != 0:
        _err("Reset failed -- see errors above.")

def interactive_menu(db_path):
    """Run the interactive menu loop matching the original C++ data_adder."""

    while True:
        print()
        print("--- Menu ---")
        print("  --- Add ---")
        print("  1. Add Vulnerability")
        print("  2. Add Vulnerability Option")
        print("  3. Add Module")
        print("  4. Add Tool")
        print("  5. Add Tool Flag")
        print("  6. Add Template")
        print("  --- View ---")
        print("  7.  View last N Vulnerabilities")
        print("  8.  View last N Modules")
        print("  9.  View last N Tools")
        print("  10. View last N Tool Flags")
        print("  11. View last N Templates")
        print("  --- Delete ---")
        print("  12. Delete Vulnerability")
        print("  13. Delete Vulnerability Option")
        print("  14. Delete Module")
        print("  15. Delete Tool")
        print("  16. Delete Tool Flag")
        print("  17. Delete Template")
        print("  ---")
        print("  18. Add Category")
        print("  19. View last N Categories")
        print("  20. Delete Category")
        print("  ---")
        print("  21. Generate Manifest")
        print("  22. Reset Database (archive old → fresh empty)")
        print("  ---")
        print("  0. Exit")

        choice = prompt("Choice", required=True)
        if choice is None:
            break

        if choice == "0":
            print("Goodbye!")
            break
        elif choice == "1":
            interactive_add_vulnerability(db_path)
        elif choice == "2":
            interactive_add_option(db_path)
        elif choice == "3":
            interactive_add_module(db_path)
        elif choice == "4":
            interactive_add_tool(db_path)
        elif choice == "5":
            interactive_add_tool_flag(db_path)
        elif choice == "6":
            interactive_add_template(db_path)
        elif choice == "7":
            interactive_view_last_n(db_path, "vulnerabilities",
                                    "Vulnerabilities",
                                    ["id", "name", "metasploit_name", "discovered_date",
                                     "discoverer", "severity", "access", "platform",
                                     "service", "description", "danger"])
        elif choice == "8":
            interactive_view_last_n(db_path, "modules", "Modules",
                                    ["id", "name", "path", "platform", "type",
                                     "description", "API", "mode", "loud", "output"])
        elif choice == "9":
            interactive_view_last_n(db_path, "tools", "Tools",
                                    ["id", "name", "category", "short_desc",
                                     "description", "flags_all"])
        elif choice == "10":
            # View last N Tool Flags — first list tools
            interactive_view_flags_or_templates(db_path, "tool_flags", "Tool Flags")
        elif choice == "11":
            # View last N Templates — first list tools
            interactive_view_flags_or_templates(db_path, "templates", "Templates")
        elif choice == "12":
            interactive_delete(db_path, "vulnerabilities", "Vulnerability")
        elif choice == "13":
            interactive_delete(db_path, "options", "Vulnerability Option")
        elif choice == "14":
            interactive_delete(db_path, "modules", "Module")
        elif choice == "15":
            interactive_delete(db_path, "tools", "Tool")
        elif choice == "16":
            interactive_delete(db_path, "tool_flags", "Tool Flag")
        elif choice == "17":
            interactive_delete(db_path, "templates", "Template")
        elif choice == "18":
            interactive_add_category(db_path)
        elif choice == "19":
            interactive_view_last_n(db_path, "categories", "Categories",
                                    ["id", "name", "display_order", "description"])
        elif choice == "20":
            interactive_delete(db_path, "categories", "Category")
        elif choice == "21":
            interactive_generate_manifest(db_path)
        elif choice == "22":
            interactive_reset(db_path)
        else:
            _warn(f"Unknown choice: {choice}")


# ──────────────────────────────────────────────
# Interactive: Add operations
# ──────────────────────────────────────────────

def interactive_add_vulnerability(db_path):
    print_header("Add Vulnerability")
    name = prompt("Name", required=True)
    metasploit_name = prompt("Metasploit Name", default="")
    discovered_date = prompt("Discovered Date", default="")
    discoverer = prompt("Discoverer", default="")
    severity = prompt("Severity", default="")
    access = prompt("Access", default="")
    platform = prompt("Platform", default="")
    service = prompt("Service", default="")
    description = prompt("Description", default="")
    danger = prompt("Danger", default="")

    if not name:
        _err("Name is required.")
        return

    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO vulnerabilities "
            "(name, metasploit_name, discovered_date, discoverer, severity, "
            " access, platform, service, description, danger) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (name, metasploit_name, discovered_date, discoverer, severity,
             access, platform, service, description, danger),
        )
        conn.commit()
        _ok(f"Vulnerability '{name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add vulnerability: {e}")
    finally:
        close_connection(conn)


def interactive_add_option(db_path):
    print_header("Add Vulnerability Option")
    # List vulnerabilities first
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name FROM vulnerabilities ORDER BY id")
        vulns = cur.fetchall()
        if not vulns:
            _warn("No vulnerabilities found. Add one first.")
            return
        print("Vulnerabilities:")
        for v in vulns:
            print(f"  {v['id']}. {v['name']}")
    finally:
        close_connection(conn)

    vuln_id = prompt_int("Vuln ID", required=True)
    option_name = prompt("Option Name", required=True)
    option_value = prompt("Option Value", required=True)

    if not option_name or not option_value:
        _err("Option name and value are required.")
        return

    conn, cur = get_connection(db_path)
    try:
        # Validate vuln_id exists
        cur.execute("SELECT id FROM vulnerabilities WHERE id = ?", (vuln_id,))
        if not cur.fetchone():
            _err(f"Vulnerability with ID {vuln_id} not found.")
            return
        cur.execute(
            "INSERT INTO options (vuln_id, option_name, option_value) "
            "VALUES (?, ?, ?)",
            (vuln_id, option_name, option_value),
        )
        conn.commit()
        _ok(f"Option '{option_name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add option: {e}")
    finally:
        close_connection(conn)


def interactive_add_module(db_path):
    print_header("Add Module")
    name = prompt("Name", required=True)
    path = prompt("Path", default="")
    platform = prompt("Platform", default="")
    mod_type = prompt("Type", default="")
    description = prompt("Description", default="")
    api = prompt("API", default="")
    mode = prompt_int("Mode (0 or 1)", default=0)
    loud = prompt_int("Loud (0 or 1)", default=0)
    output = prompt("Output", default="")

    if not name:
        _err("Name is required.")
        return

    # If mode=0, force loud=0 (matching C++ logic)
    if mode == 0:
        loud = 0

    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO modules "
            "(name, path, platform, type, description, API, mode, loud, output) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (name, path, platform, mod_type, description, api, mode, loud, output),
        )
        conn.commit()
        _ok(f"Module '{name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add module: {e}")
    finally:
        close_connection(conn)


def interactive_add_tool(db_path):
    print_header("Add Tool")
    name = prompt("Name", required=True)

    # List existing categories
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT name FROM categories ORDER BY display_order")
        cats = [r["name"] for r in cur.fetchall()]
        if cats:
            print("Available categories:")
            for c in cats:
                print(f"  - {c}")
    finally:
        close_connection(conn)

    category = prompt("Category", default="")
    short_desc = prompt("Short Description", default="")
    description = prompt("Description", default="")
    flags_all = prompt("Flags All", default="")

    if not name:
        _err("Name is required.")
        return

    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO tools (name, category, short_desc, description, flags_all) "
            "VALUES (?, ?, ?, ?, ?)",
            (name, category, short_desc, description, flags_all),
        )
        conn.commit()
        _ok(f"Tool '{name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add tool: {e}")
    finally:
        close_connection(conn)


def interactive_add_tool_flag(db_path):
    print_header("Add Tool Flag")
    # List all tools
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name FROM tools ORDER BY id")
        tools = cur.fetchall()
        if not tools:
            _warn("No tools found. Add one first.")
            return
        print("Tools:")
        for t in tools:
            print(f"  {t['id']}. {t['name']}")
    finally:
        close_connection(conn)

    tool_id = prompt_int("Tool ID", required=True)
    name = prompt("Flag Name", required=True)
    description = prompt("Description", default="")
    loud_bool = prompt_bool("Loud", default="n")
    loud = "yes" if loud_bool else "no"
    root = prompt_bool("Root (requires root?)", default="n")
    protocols = prompt("Protocols", default="")

    if not name:
        _err("Flag name is required.")
        return

    root_val = 1 if root else 0

    conn, cur = get_connection(db_path)
    try:
        # Validate tool_id
        cur.execute("SELECT id FROM tools WHERE id = ?", (tool_id,))
        if not cur.fetchone():
            _err(f"Tool with ID {tool_id} not found.")
            return
        cur.execute(
            "INSERT INTO tool_flags (tool_id, name, description, loud, root, protocols) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (tool_id, name, description, loud, root_val, protocols),
        )
        conn.commit()
        _ok(f"Tool flag '{name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add tool flag: {e}")
    finally:
        close_connection(conn)


def interactive_add_template(db_path):
    print_header("Add Template")
    # List all tools
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name FROM tools ORDER BY id")
        tools = cur.fetchall()
        if not tools:
            _warn("No tools found. Add one first.")
            return
        print("Tools:")
        for t in tools:
            print(f"  {t['id']}. {t['name']}")
    finally:
        close_connection(conn)

    tool_id = prompt_int("Tool ID", required=True)
    template_name = prompt("Template Name", required=True)
    description = prompt("Description", default="")
    root = prompt_bool("Root (requires root?)", default="n")
    protocols = prompt("Protocols", default="")
    flag = prompt_bool("Flag (has flags?)", default="y")

    if not template_name:
        _err("Template name is required.")
        return

    root_val = 1 if root else 0
    flag_val = 1 if flag else 0

    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id FROM tools WHERE id = ?", (tool_id,))
        if not cur.fetchone():
            _err(f"Tool with ID {tool_id} not found.")
            return
        cur.execute(
            "INSERT INTO templates (tool_id, template_name, description, root, protocols, flag) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (tool_id, template_name, description, root_val, protocols, flag_val),
        )
        conn.commit()
        _ok(f"Template '{template_name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add template: {e}")
    finally:
        close_connection(conn)


def interactive_add_category(db_path):
    print_header("Add Category")
    name = prompt("Name", required=True)
    display_order = prompt_int("Display Order", default=0)
    description = prompt("Description", default="")

    if not name:
        _err("Name is required.")
        return

    conn, cur = get_connection(db_path)
    try:
        cur.execute(
            "INSERT INTO categories (name, display_order, description) "
            "VALUES (?, ?, ?)",
            (name, display_order, description),
        )
        conn.commit()
        _ok(f"Category '{name}' added with ID {cur.lastrowid}.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to add category: {e}")
    finally:
        close_connection(conn)


# ──────────────────────────────────────────────
# Interactive: View last N
# ──────────────────────────────────────────────

def interactive_view_last_n(db_path, table, label, columns):
    """Show the last N records from a table."""
    n = prompt_int("How many to show", default=5)
    if n is None or n <= 0:
        return

    conn, cur = get_connection(db_path)
    try:
        col_list = ", ".join(columns)
        cur.execute(f"SELECT {col_list} FROM {table} ORDER BY id DESC LIMIT ?", (n,))
        rows = cur.fetchall()
        if not rows:
            _warn(f"No {label.lower()} found.")
            return

        print()
        print_separator()
        print(f"  Last {len(rows)} {label}")
        print_separator()
        for r in rows:
            for i, col in enumerate(columns):
                val = r[i]
                # Format display_name for labels
                display = col.replace("_", " ").title()
                if col == "id":
                    print(f"  {'ID':<20}: {val}")
                elif col == "mode" or col == "loud" or col == "root" or col == "flag":
                    print(f"  {display:<20}: {val}")
                else:
                    # Truncate very long text
                    sval = str(val) if val is not None else ""
                    if len(sval) > 120:
                        sval = sval[:117] + "..."
                    print(f"  {display:<20}: {sval}")
            print()
    finally:
        close_connection(conn)


def interactive_view_flags_or_templates(db_path, table, label):
    """View flags or templates — list tools first, then show last N filtered by tool_id."""
    conn, cur = get_connection(db_path)
    try:
        cur.execute("SELECT id, name FROM tools ORDER BY id")
        tools = cur.fetchall()
        if not tools:
            _warn("No tools found.")
            return
        print("Tools:")
        for t in tools:
            cur2 = conn.cursor()
            cur2.execute(f"SELECT COUNT(*) FROM {table} WHERE tool_id = ?", (t["id"],))
            cnt = cur2.fetchone()[0]
            print(f"  {t['id']}. {t['name']} ({cnt} {label.lower()})")
    finally:
        close_connection(conn)

    tool_id = prompt_int("Tool ID", required=True)
    if tool_id is None:
        return
    n = prompt_int("How many to show", default=5)
    if n is None or n <= 0:
        return

    conn, cur = get_connection(db_path)
    try:
        if table == "tool_flags":
            columns = ["id", "tool_id", "name", "description", "loud", "root", "protocols"]
        else:
            columns = ["id", "tool_id", "template_name", "description", "root", "protocols", "flag"]

        col_list = ", ".join(columns)
        cur.execute(
            f"SELECT {col_list} FROM {table} WHERE tool_id = ? ORDER BY id DESC LIMIT ?",
            (tool_id, n),
        )
        rows = cur.fetchall()
        if not rows:
            _warn(f"No {label.lower()} found for tool ID {tool_id}.")
            return

        print()
        print_separator()
        print(f"  Last {len(rows)} {label} for tool ID {tool_id}")
        print_separator()
        for r in rows:
            for i, col in enumerate(columns):
                val = r[i]
                display = col.replace("_", " ").title()
                if col == "id" or col == "tool_id":
                    print(f"  {'ID' if col == 'id' else 'Tool ID':<20}: {val}")
                else:
                    sval = str(val) if val is not None else ""
                    if len(sval) > 120:
                        sval = sval[:117] + "..."
                    print(f"  {display:<20}: {sval}")
            print()
    finally:
        close_connection(conn)


# ──────────────────────────────────────────────
# Interactive: Delete
# ──────────────────────────────────────────────

def interactive_delete(db_path, table, label):
    """List all records in a table and ask which ID to delete."""
    conn, cur = get_connection(db_path)
    try:
        # Determine display columns based on table
        if table == "vulnerabilities":
            cur.execute("SELECT id, name, severity FROM vulnerabilities ORDER BY id")
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. {r['name']} ({r['severity']})")
        elif table == "options":
            cur.execute(
                "SELECT o.id, v.name AS vuln_name, o.option_name "
                "FROM options o "
                "LEFT JOIN vulnerabilities v ON v.id = o.vuln_id "
                "ORDER BY o.id"
            )
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. [{r['vuln_name']}] {r['option_name']}")
        elif table == "modules":
            cur.execute("SELECT id, name, platform FROM modules ORDER BY id")
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. {r['name']} ({r['platform']})")
        elif table == "tools":
            cur.execute("SELECT id, name, category FROM tools ORDER BY id")
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. {r['name']} [{r['category']}]")
        elif table == "tool_flags":
            cur.execute(
                "SELECT f.id, t.name AS tool_name, f.name AS flag_name "
                "FROM tool_flags f "
                "LEFT JOIN tools t ON t.id = f.tool_id "
                "ORDER BY f.id"
            )
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. [{r['tool_name']}] {r['flag_name']}")
        elif table == "templates":
            cur.execute(
                "SELECT tm.id, t.name AS tool_name, tm.template_name "
                "FROM templates tm "
                "LEFT JOIN tools t ON t.id = tm.tool_id "
                "ORDER BY tm.id"
            )
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. [{r['tool_name']}] {r['template_name']}")
        elif table == "categories":
            cur.execute("SELECT id, name, display_order FROM categories ORDER BY id")
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}. {r['name']} (order: {r['display_order']})")
        else:
            cur.execute(f"SELECT id FROM {table} ORDER BY id")
            rows = cur.fetchall()
            if not rows:
                _warn(f"No {label.lower()}s found.")
                return
            print(f"\n{label}s:")
            for r in rows:
                print(f"  {r['id']}.")
    finally:
        close_connection(conn)

    id_val = prompt_int("ID to delete", required=True)
    if id_val is None:
        return

    if not confirm(f"Delete {label.lower()} ID {id_val}?"):
        _info("Cancelled.")
        return

    conn, cur = get_connection(db_path)
    try:
        cur.execute(f"SELECT id FROM {table} WHERE id = ?", (id_val,))
        if not cur.fetchone():
            _err(f"{label} with ID {id_val} not found.")
            return
        cur.execute(f"DELETE FROM {table} WHERE id = ?", (id_val,))
        conn.commit()
        _ok(f"{label} ID {id_val} deleted.")
    except sqlite3.Error as e:
        conn.rollback()
        _err(f"Failed to delete: {e}")
    finally:
        close_connection(conn)


def interactive_generate_manifest(db_path):
    """Generate signed_manifest.json interactively."""
    version = prompt("Version", default=DEFAULT_VERSION)
    output_path = prompt("Output path", default=DEFAULT_MANIFEST_PATH)
    db_url = prompt("DB URL", default=DEFAULT_DB_URL)

    if not os.path.isfile(db_path):
        _err(f"Database file not found: {db_path}")
        return

    if not version:
        _err("Version string must not be empty.")
        return

    if not db_url.startswith("https://"):
        _err("db_url must start with https://")
        return

    db_hash = _sha256_file(db_path)

    manifest = {
        "version": version,
        "db_hash": db_hash,
        "db_url": db_url,
    }

    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    _ok(f"Manifest written to: {output_path}")
    print(f"    version  : {version}")
    print(f"    db_hash  : {db_hash}")
    print(f"    db_url   : {db_url}")


# ──────────────────────────────────────────────
# CLI argument parser
# ──────────────────────────────────────────────

def build_parser():
    parser = argparse.ArgumentParser(
        prog="data_adder.py",
        description="Tguide Data Adder — Interactive & CLI tool for managing tguide.db",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              # Interactive menu mode
              %(prog)s

              # CLI add (tool)
              %(prog)s add tool --name nmap --category "Network Scanning" --short-desc "..." --description "..." --flags-all "-sS -sV"

              # CLI add (vulnerability)
              %(prog)s add vulnerability --name "CVE-2024-xxx" --severity high --platform linux ...

              # CLI add (module)
              %(prog)s add module --name shodan_ip --path /recon/... --platform shodan --type recon --description "..." --api shodan_api --mode true --loud false --output contacts

              # CLI add (flag)
              %(prog)s add flag --tool-id 1 --name "-sS" --description "SYN scan" --loud no --root false --protocols tcp

              # CLI add (template)
              %(prog)s add template --tool-id 1 --template-name "Quick Scan" --description "Scans top ports" --root false --protocols tcp --flag true

              # CLI add (category)
              %(prog)s add category --name "New Category" --display-order 9 --description "Description"

              # CLI add (option)
              %(prog)s add option --vuln-id 1 --option-name RHOSTS --option-value "<target_ip>"

              # CLI list
              %(prog)s list tools
              %(prog)s list vulnerabilities
              %(prog)s list modules
              %(prog)s list categories

              # CLI delete
              %(prog)s delete tool --id 5
              %(prog)s delete vulnerability --id 3
              %(prog)s delete flag --id 2
              %(prog)s delete template --id 1
              %(prog)s delete category --id 8
              %(prog)s delete option --id 7

              # CLI manifest
              %(prog)s manifest --version 1.1.0

              # CLI reset (archive old database, create fresh empty)
              %(prog)s reset
        """),
    )

    parser.add_argument(
        "--db",
        default=None,
        help=f"Path to SQLite database (default: {DEFAULT_DB_PATH})",
    )

    subparsers = parser.add_subparsers(
        dest="command",
        title="Commands",
        description="Available subcommands",
    )

    # ── add ──
    add_parser = subparsers.add_parser("add", help="Add a record to the database")
    add_sub = add_parser.add_subparsers(dest="add_type", title="Type of record to add")

    # add tool
    p = add_sub.add_parser("tool", help="Add a tool")
    p.add_argument("--name", required=True)
    p.add_argument("--category", default="")
    p.add_argument("--short-desc", default="")
    p.add_argument("--description", default="")
    p.add_argument("--flags-all", default="")

    # add vulnerability
    p = add_sub.add_parser("vulnerability", help="Add a vulnerability")
    p.add_argument("--name", required=True)
    p.add_argument("--metasploit-name", default="")
    p.add_argument("--discovered-date", default="")
    p.add_argument("--discoverer", default="")
    p.add_argument("--severity", default="")
    p.add_argument("--access", default="")
    p.add_argument("--platform", default="")
    p.add_argument("--service", default="")
    p.add_argument("--description", default="")
    p.add_argument("--danger", default="")

    # add module
    p = add_sub.add_parser("module", help="Add a module")
    p.add_argument("--name", required=True)
    p.add_argument("--path", default="")
    p.add_argument("--platform", default="")
    p.add_argument("--type", default="")
    p.add_argument("--description", default="")
    p.add_argument("--api", default="")
    p.add_argument("--mode", type=lambda x: x.lower() in ("true", "1", "yes"), default=False)
    p.add_argument("--loud", type=lambda x: x.lower() in ("true", "1", "yes"), default=False)
    p.add_argument("--output", default="")

    # add flag
    p = add_sub.add_parser("flag", help="Add a tool flag")
    p.add_argument("--tool-id", type=int, required=True)
    p.add_argument("--name", required=True)
    p.add_argument("--description", default="")
    p.add_argument("--loud", default="no")
    p.add_argument("--root", type=lambda x: x.lower() in ("true", "1", "yes"), default=False)
    p.add_argument("--protocols", default="")

    # add template
    p = add_sub.add_parser("template", help="Add a template")
    p.add_argument("--tool-id", type=int, required=True)
    p.add_argument("--template-name", required=True)
    p.add_argument("--description", default="")
    p.add_argument("--root", type=lambda x: x.lower() in ("true", "1", "yes"), default=False)
    p.add_argument("--protocols", default="")
    p.add_argument("--flag", type=lambda x: x.lower() in ("true", "1", "yes"), default=False)

    # add category
    p = add_sub.add_parser("category", help="Add a category")
    p.add_argument("--name", required=True)
    p.add_argument("--display-order", type=int, default=0)
    p.add_argument("--description", default="")

    # add option
    p = add_sub.add_parser("option", help="Add a vulnerability option")
    p.add_argument("--vuln-id", type=int, required=True)
    p.add_argument("--option-name", required=True)
    p.add_argument("--option-value", required=True)

    # ── list ──
    list_parser = subparsers.add_parser("list", help="List records from the database")
    list_sub = list_parser.add_subparsers(dest="list_type", title="Type of records to list")
    list_sub.add_parser("tools", help="List all tools")
    list_sub.add_parser("vulnerabilities", help="List all vulnerabilities")
    list_sub.add_parser("modules", help="List all modules")
    list_sub.add_parser("categories", help="List all categories")

    # ── delete ──
    del_parser = subparsers.add_parser("delete", help="Delete a record from the database")
    del_sub = del_parser.add_subparsers(dest="delete_type", title="Type of record to delete")
    for tname in ("tool", "vulnerability", "module", "flag", "template", "category", "option"):
        p = del_sub.add_parser(tname, help=f"Delete a {tname}")
        p.add_argument("--id", type=int, required=True)

    # ── reset ──
    reset_parser = subparsers.add_parser("reset", help="Archive database to old_data/ and create fresh empty one")
    reset_parser.add_argument(
        "--force", "-f",
        action="store_true",
        help="Skip confirmation prompt (for scripting)",
    )

    # ── manifest ──
    manifest_parser = subparsers.add_parser("manifest", help="Generate signed_manifest.json")
    manifest_parser.add_argument(
        "--manifest",
        default=DEFAULT_MANIFEST_PATH,
        help=f"Output path for manifest (default: {DEFAULT_MANIFEST_PATH})",
    )
    manifest_parser.add_argument(
        "--version",
        default=DEFAULT_VERSION,
        help=f"Version string (default: {DEFAULT_VERSION})",
    )
    manifest_parser.add_argument(
        "--db-url",
        default=DEFAULT_DB_URL,
        help=f"Download URL (default: {DEFAULT_DB_URL})",
    )

    return parser


# ──────────────────────────────────────────────
# Main entry point
# ──────────────────────────────────────────────

def main():
    Colors.init()

    parser = build_parser()
    args = parser.parse_args()

    # Determine db_path (global --db or default)
    db_path = args.db if args.db else DEFAULT_DB_PATH

    # If no subcommand, run interactive menu
    if args.command is None:
        # Check DB exists
        if not os.path.isfile(db_path):
            _err(f"Database not found at '{db_path}'.")
            _err("Run 'python3 tools/build_db.py init' and 'python3 tools/build_db.py build' first.")
            return 1
        interactive_menu(db_path)
        return 0

    # Subcommand routing
    if args.command == "add":
        if args.add_type is None:
            parser.parse_args(["add", "--help"])
            return 1
        handlers = {
            "tool": cmd_add_tool,
            "vulnerability": cmd_add_vulnerability,
            "module": cmd_add_module,
            "flag": cmd_add_flag,
            "template": cmd_add_template,
            "category": cmd_add_category,
            "option": cmd_add_option,
        }
        handler = handlers.get(args.add_type)
        if handler:
            handler(db_path, args)
        return 0

    elif args.command == "list":
        if args.list_type is None:
            parser.parse_args(["list", "--help"])
            return 1
        handlers = {
            "tools": cmd_list_tools,
            "vulnerabilities": cmd_list_vulnerabilities,
            "modules": cmd_list_modules,
            "categories": cmd_list_categories,
        }
        handler = handlers.get(args.list_type)
        if handler:
            handler(db_path)
        return 0

    elif args.command == "delete":
        if args.delete_type is None:
            parser.parse_args(["delete", "--help"])
            return 1
        table_map = {
            "tool": "tools",
            "vulnerability": "vulnerabilities",
            "module": "modules",
            "flag": "tool_flags",
            "template": "templates",
            "category": "categories",
            "option": "options",
        }
        label_map = {
            "tool": "Tool",
            "vulnerability": "Vulnerability",
            "module": "Module",
            "flag": "Tool Flag",
            "template": "Template",
            "category": "Category",
            "option": "Vulnerability Option",
        }
        table = table_map.get(args.delete_type)
        label = label_map.get(args.delete_type, "Record")
        if table:
            cmd_delete_by_id(db_path, table, args.id, label)
        return 0

    elif args.command == "reset":
        return cmd_reset(db_path, force=args.force)

    elif args.command == "manifest":
        cmd_manifest(db_path, args)
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
