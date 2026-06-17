"""
Tguide Database Utilities — validate and dump

The seed database (data/database/tguide.db) is committed as a pre-built
SQLite file, not built from YAML during the build process.
This module provides helper functions for validating the DB schema and
dumping its contents for developer inspection.

Usage (internal — called by build_db.py):
    validate_database(db_path)         →  list of issues (empty = OK)
    dump_database(db_path)             →  formatted text
"""

import os
import sqlite3


def _assert_valid_table(tbl, table_defs):
    """Assert that tbl is a known table name from the schema definitions."""
    valid = {t["name"] for t in table_defs}
    assert tbl in valid, f"Unexpected table name: {tbl!r} (valid: {sorted(valid)})"






# ──────────────────────────────────────────────
# Validation
# ──────────────────────────────────────────────

def validate_database(db_path):
    """
    Validate an existing SQLite database against the expected schema.

    Checks:
      - File exists and has SQLite magic header
      - All 7 tables exist
      - All required columns exist per table
      - Tables contain data (warning only)

    Returns:
        List of issue strings (empty = everything is valid).
    """
    from schema import get_all_tables

    issues = []

    if not os.path.isfile(db_path):
        return [f"Database file not found: {db_path}"]

    # ── Check SQLite magic header ──
    try:
        with open(db_path, "rb") as f:
            header = f.read(16)
    except OSError as e:
        return [f"Cannot read database file: {e}"]
    SQLITE_MAGIC = b"SQLite format 3\x00"
    if header[:16] != SQLITE_MAGIC:
        issues.append(f"Not a valid SQLite database (bad magic header): {db_path}")
        return issues  # can't proceed further

    # ── Connect and validate schema ──
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        # Check all tables exist
        cursor.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
        )
        existing_tables = {row[0] for row in cursor.fetchall()}
        expected_tables = {t["name"] for t in get_all_tables()}

        for tbl in sorted(expected_tables):
            if tbl not in existing_tables:
                issues.append(f"Missing table: '{tbl}'")

        # Check columns per table
        for tdef in get_all_tables():
            tbl = tdef["name"]
            if tbl not in existing_tables:
                continue

            _assert_valid_table(tbl, get_all_tables())
            cursor.execute(f"PRAGMA table_info({tbl})")
            actual_cols = {row[1] for row in cursor.fetchall()}

            for col_name, col_type, _ in tdef["columns"]:
                if col_name not in actual_cols:
                    issues.append(f"Table '{tbl}': missing column '{col_name}'")

        # Check for data (warning)
        for tdef in get_all_tables():
            tbl = tdef["name"]
            if tbl not in existing_tables:
                continue
            _assert_valid_table(tbl, get_all_tables())
            cursor.execute(f"SELECT COUNT(*) FROM {tbl}")
            count = cursor.fetchone()[0]
            if count == 0:
                # translations can legitimately be empty
                if tbl != "translations":
                    issues.append(f"Table '{tbl}' is empty (warning)")

        conn.close()

    except sqlite3.Error as e:
        issues.append(f"SQLite error during validation: {e}")

    return issues


# ──────────────────────────────────────────────
# Dump
# ──────────────────────────────────────────────

def dump_database(db_path):
    """
    Return a formatted text dump of the entire database contents.

    Args:
        db_path: Path to the SQLite database.

    Returns:
        Formatted string with all table contents.
    """
    from schema import get_all_tables, INSERTION_ORDER

    if not os.path.isfile(db_path):
        return f"[ERROR] Database file not found: {db_path}"

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    lines = []
    lines.append("=" * 72)
    lines.append("  Tguide Database Dump")
    lines.append(f"  Database: {db_path}")
    lines.append("=" * 72)
    lines.append("")

    # Dump in insertion order for readability
    ordered_tables = [t for t in get_all_tables() if t["name"] in INSERTION_ORDER]
    # Also include any tables not in insertion order
    extra = [t for t in get_all_tables() if t["name"] not in INSERTION_ORDER]
    ordered_tables.extend(extra)

    for tdef in ordered_tables:
        tbl = tdef["name"]
        col_names = [c[0] for c in tdef["columns"]]

        try:
            _assert_valid_table(tbl, get_all_tables())
            cursor.execute(f"SELECT * FROM {tbl} ORDER BY id")
            rows = cursor.fetchall()
        except sqlite3.Error as e:
            lines.append(f"[ERROR] Table '{tbl}': {e}")
            continue

        lines.append(f"─── {tbl} ({len(rows)} rows) ───")
        lines.append("")

        if not rows:
            lines.append("  (empty)")
            lines.append("")
            continue

        # Calculate column widths
        col_widths = []
        for i, name in enumerate(col_names):
            w = max(len(name), 4)
            for row in rows:
                val = str(row[i]) if row[i] is not None else "NULL"
                w = max(w, len(val))
            col_widths.append(min(w, 40))  # cap at 40 chars

        # Header
        header_parts = []
        for i, name in enumerate(col_names):
            header_parts.append(name.ljust(col_widths[i]))
        lines.append("  " + " | ".join(header_parts))
        lines.append("  " + "-" * (sum(col_widths) + 3 * (len(col_widths) - 1)))

        # Rows
        for row in rows:
            parts = []
            for i, val in enumerate(row):
                s = str(val) if val is not None else "NULL"
                if len(s) > 40:
                    s = s[:37] + "..."
                parts.append(s.ljust(col_widths[i]))
            lines.append("  " + " | ".join(parts))

        lines.append("")

    conn.close()
    return "\n".join(lines)
