"""
Tguide Database Builder

Builds, validates, and dumps the tguide.db SQLite database
from YAML/JSON data files.

Usage (internal — called by build_db.py):
    build_database(db_path, data_dir)  →  path to built DB
    validate_database(db_path)         →  list of issues (empty = OK)
    dump_database(db_path)             →  formatted text
"""

import hashlib
import json
import os
import sqlite3
import sys
import textwrap


def _assert_valid_table(tbl, table_defs):
    """Assert that tbl is a known table name from the schema definitions."""
    valid = {t["name"] for t in table_defs}
    assert tbl in valid, f"Unexpected table name: {tbl!r} (valid: {sorted(valid)})"


# ──────────────────────────────────────────────
# YAML/JSON loader — supports both formats
# ──────────────────────────────────────────────

_HAS_YAML = False
try:
    import yaml as _yaml_lib
    _HAS_YAML = True
except ImportError:
    pass


def _load_data_file(filepath):
    """Load a YAML or JSON data file.  Raises on error."""
    if not os.path.isfile(filepath):
        raise FileNotFoundError(f"Data file not found: {filepath}")

    ext = os.path.splitext(filepath)[1].lower()

    if ext in (".json",):
        with open(filepath, "r", encoding="utf-8") as f:
            return json.load(f)

    if ext in (".yaml", ".yml"):
        if _HAS_YAML:
            with open(filepath, "r", encoding="utf-8") as f:
                data = _yaml_lib.safe_load(f)
                if data is None:
                    data = {}
                return data
        else:
            # Should not be reached if _load_data_dir filters properly,
            # but handle defensively.
            raise RuntimeError(
                f"Cannot load {filepath}: PyYAML is not installed.\n"
                f"  Install it with:  pip install pyyaml\n"
                f"  Or convert the file to JSON format."
            )

    raise ValueError(f"Unsupported file extension: {ext} (expected .json, .yaml, or .yml)")


def _load_data_dir(data_dir, basename):
    """Load a data file by basename (without extension), trying .yaml then .json.

    If PyYAML is not installed, .yaml/.yml files are silently skipped
    and only .json files are loaded.
    """
    # Determine which extensions are loadable
    if _HAS_YAML:
        loadable_exts = (".yaml", ".yml", ".json")
    else:
        loadable_exts = (".json",)

    for ext in loadable_exts:
        p = os.path.join(data_dir, basename + ext)
        if os.path.isfile(p):
            return _load_data_file(p)

    tried = [basename + e for e in (".yaml", ".yml", ".json")]
    raise FileNotFoundError(
        f"Could not find {basename} data file in {data_dir} "
        f"(tried {', '.join(tried)})"
    )


# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────

def _to_bool(val):
    """Convert a value to a SQLite integer boolean (0 or 1)."""
    if isinstance(val, bool):
        return 1 if val else 0
    if isinstance(val, int):
        return 1 if val else 0
    if isinstance(val, str):
        v = val.strip().lower()
        if v in ("1", "true", "yes", "y"):
            return 1
        if v in ("0", "false", "no", "n"):
            return 0
        raise ValueError(f"Cannot convert to boolean: {val!r}")
    raise ValueError(f"Unexpected type for boolean: {type(val).__name__} {val!r}")


def _to_int(val, field_name="value"):
    """Convert a value to int, raising a clear error on failure."""
    if isinstance(val, int):
        return val
    if isinstance(val, str):
        try:
            return int(val)
        except ValueError:
            raise ValueError(f"Field '{field_name}': cannot convert {val!r} to int")
    raise ValueError(f"Field '{field_name}': unexpected type {type(val).__name__} ({val!r})")


# ──────────────────────────────────────────────
# Data loading & validation
# ──────────────────────────────────────────────

def _check_required(data, required_fields, label=""):
    """Validate that required fields are present and non-empty."""
    issues = []
    for field in required_fields:
        val = data.get(field)
        if val is None or (isinstance(val, str) and val.strip() == ""):
            issues.append(f"{label}: missing or empty required field '{field}'")
    return issues


def _load_and_validate_categories(data_dir):
    """Load categories from YAML/JSON and validate."""
    raw = _load_data_dir(data_dir, "categories")
    items = raw.get("categories", [])
    issues = []
    for i, cat in enumerate(items):
        issues += _check_required(cat, ["name", "display_order"], f"categories[{i}]")
        if "display_order" in cat:
            try:
                _to_int(cat["display_order"], "display_order")
            except ValueError as e:
                issues.append(f"categories[{i}]: {e}")
    return items, issues


def _load_and_validate_tools(data_dir):
    """Load tools from YAML/JSON and validate."""
    raw = _load_data_dir(data_dir, "tools")
    items = raw.get("tools", [])
    issues = []
    for i, t in enumerate(items):
        issues += _check_required(t, ["name", "category"], f"tools[{i}]")
    return items, issues


def _load_and_validate_tool_flags(data_dir, tools_by_name):
    """Load tool_flags from YAML/JSON and validate FK references."""
    raw = _load_data_dir(data_dir, "tool_flags")
    items = raw.get("tool_flags", [])
    issues = []
    for i, f in enumerate(items):
        issues += _check_required(f, ["tool_name", "name"], f"tool_flags[{i}]")
        if "tool_name" in f and f["tool_name"] not in tools_by_name:
            issues.append(
                f"tool_flags[{i}]: references unknown tool '{f.get('tool_name')}'"
            )
        if "root" in f:
            try:
                _to_bool(f["root"])
            except ValueError as e:
                issues.append(f"tool_flags[{i}]: root — {e}")
    return items, issues


def _load_and_validate_templates(data_dir, tools_by_name):
    """Load templates from YAML/JSON and validate FK references."""
    raw = _load_data_dir(data_dir, "templates")
    items = raw.get("templates", [])
    issues = []
    for i, t in enumerate(items):
        issues += _check_required(
            t, ["tool_name", "template_name"], f"templates[{i}]"
        )
        if "tool_name" in t and t["tool_name"] not in tools_by_name:
            issues.append(
                f"templates[{i}]: references unknown tool '{t.get('tool_name')}'"
            )
        if "root" in t:
            try:
                _to_bool(t["root"])
            except ValueError as e:
                issues.append(f"templates[{i}]: root — {e}")
        if "flag" in t:
            try:
                _to_bool(t["flag"])
            except ValueError as e:
                issues.append(f"templates[{i}]: flag — {e}")
    return items, issues


def _load_and_validate_modules(data_dir):
    """Load modules from YAML/JSON and validate."""
    raw = _load_data_dir(data_dir, "modules")
    items = raw.get("modules", [])
    issues = []
    for i, m in enumerate(items):
        issues += _check_required(m, ["name", "path", "platform"], f"modules[{i}]")
        if "mode" in m:
            try:
                _to_bool(m["mode"])
            except ValueError as e:
                issues.append(f"modules[{i}]: mode — {e}")
        if "loud" in m:
            try:
                _to_bool(m["loud"])
            except ValueError as e:
                issues.append(f"modules[{i}]: loud — {e}")
    return items, issues


def _load_and_validate_vulnerabilities(data_dir):
    """Load vulnerabilities and options from YAML/JSON."""
    raw = _load_data_dir(data_dir, "vulnerabilities")
    vulns = raw.get("vulnerabilities", [])
    opts = raw.get("options", [])
    issues = []
    for i, v in enumerate(vulns):
        issues += _check_required(v, ["name"], f"vulnerabilities[{i}]")
    # Validate options reference existing vulns
    vuln_names = {v["name"] for v in vulns}
    for i, o in enumerate(opts):
        issues += _check_required(o, ["vuln_name", "option_name"], f"options[{i}]")
        if "vuln_name" in o and o["vuln_name"] not in vuln_names:
            issues.append(
                f"options[{i}]: references unknown vulnerability '{o.get('vuln_name')}'"
            )
    return vulns, opts, issues


# ──────────────────────────────────────────────
# Database building
# ──────────────────────────────────────────────


# Which columns can be translated per content table
TRANSLATABLE_COLUMNS = {
    "tools":            {"name", "short_desc", "description"},
    "categories":       {"name", "description"},
    "vulnerabilities":  {"name", "description", "metasploit_name"},
    "modules":          {"name", "description"},
    "tool_flags":       {"name", "description"},
    "templates":        {"template_name", "description"},
}


def _load_and_validate_translations(data_dir, known_ids):
    """Load translations from YAML/JSON and validate FK references.

    known_ids is a dict: {table_name: {name_lookup: row_id}}
    e.g. {"tools": {"nmap": 1, "netcat": 2}, ...}
    Translation entries reference rows by name, then get resolved to IDs.
    """
    raw = _load_data_dir(data_dir, "translations")
    items = raw.get("translations", [])
    issues = []
    for i, tr in enumerate(items):
        issues += _check_required(
            tr, ["table_name", "row_name", "column_name", "lang", "value"],
            f"translations[{i}]"
        )
        tbl = tr.get("table_name", "")
        if tbl and tbl not in known_ids:
            issues.append(f"translations[{i}]: unknown table '{tbl}'")
            continue

        # Validate column name against translatable columns whitelist
        col = tr.get("column_name", "")
        if tbl and col:
            allowed = TRANSLATABLE_COLUMNS.get(tbl, set())
            if col not in allowed:
                issues.append(
                    f"translations[{i}]: column '{col}' is not translatable "
                    f"for table '{tbl}' (allowed: {sorted(allowed)})"
                )
                continue

        row_name = tr.get("row_name", "")
        if tbl and row_name and row_name not in known_ids.get(tbl, {}):
            issues.append(
                f"translations[{i}]: references unknown {tbl} row '{row_name}'"
            )
    return items, issues


def build_database(db_path, data_dir):
    """
    Build the tguide.db from YAML/JSON data files.

    Returns:
        (db_path, issues)  — issues is a list of error/warning strings.
                             If non-empty, the DB may be incomplete.
    """
    from schema import create_tables, INSERTION_ORDER

    all_issues = []

    # ── Ensure parent directory exists ──
    os.makedirs(os.path.dirname(db_path) or ".", exist_ok=True)

    # ── Load and validate all data ──
    categories, cat_issues = _load_and_validate_categories(data_dir)
    all_issues += cat_issues

    tools, tool_issues = _load_and_validate_tools(data_dir)
    all_issues += tool_issues

    tools_by_name = {t["name"]: t for t in tools}

    tool_flags, tf_issues = _load_and_validate_tool_flags(data_dir, tools_by_name)
    all_issues += tf_issues

    templates, tmpl_issues = _load_and_validate_templates(data_dir, tools_by_name)
    all_issues += tmpl_issues

    modules, mod_issues = _load_and_validate_modules(data_dir)
    all_issues += mod_issues

    vulns, opts, vuln_issues = _load_and_validate_vulnerabilities(data_dir)
    all_issues += vuln_issues

    # ── Remove existing database for clean rebuild ──
    if os.path.isfile(db_path):
        os.remove(db_path)

    # ── Open / create database ──
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA foreign_keys = ON;")
    conn.execute("PRAGMA journal_mode = WAL;")
    cursor = conn.cursor()

    try:
        # ── Create tables ──
        create_tables(cursor)

        # ── Insert data in dependency order ──
        # 1. categories
        for cat in categories:
            cursor.execute(
                "INSERT INTO categories(name, display_order, description) VALUES (?, ?, ?)",
                (cat["name"], _to_int(cat["display_order"]), cat.get("description", "")),
            )

        # 2. tools
        for tool in tools:
            cursor.execute(
                "INSERT INTO tools(name, category, short_desc, description, flags_all) "
                "VALUES (?, ?, ?, ?, ?)",
                (
                    tool["name"],
                    tool["category"],
                    tool.get("short_desc", ""),
                    tool.get("description", ""),
                    tool.get("flags_all", ""),
                ),
            )

        # Build tool_id lookup (after insert so we get real rowids)
        tool_ids = {}
        for tool in tools:
            cursor.execute("SELECT id FROM tools WHERE name = ?", (tool["name"],))
            row = cursor.fetchone()
            if row:
                tool_ids[tool["name"]] = row[0]

        # 3. tool_flags
        for tf in tool_flags:
            tid = tool_ids.get(tf["tool_name"])
            if tid is None:
                continue  # already reported as validation issue
            cursor.execute(
                "INSERT INTO tool_flags(tool_id, name, description, loud, root, protocols) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                (
                    tid,
                    tf["name"],
                    tf.get("description", ""),
                    tf.get("loud", ""),
                    _to_bool(tf.get("root", False)),
                    tf.get("protocols", ""),
                ),
            )

        # 4. templates
        for tmpl in templates:
            tid = tool_ids.get(tmpl["tool_name"])
            if tid is None:
                continue
            cursor.execute(
                "INSERT INTO templates(tool_id, template_name, description, root, protocols, flag) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                (
                    tid,
                    tmpl["template_name"],
                    tmpl.get("description", ""),
                    _to_bool(tmpl.get("root", False)),
                    tmpl.get("protocols", ""),
                    _to_bool(tmpl.get("flag", False)),
                ),
            )

        # 5. vulnerabilities
        vuln_ids = {}
        for v in vulns:
            cursor.execute(
                "INSERT INTO vulnerabilities"
                "(name, metasploit_name, discovered_date, discoverer,"
                " severity, access, platform, service, description, danger) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (
                    v["name"],
                    v.get("metasploit_name", ""),
                    v.get("discovered_date", ""),
                    v.get("discoverer", ""),
                    v.get("severity", ""),
                    v.get("access", ""),
                    v.get("platform", ""),
                    v.get("service", ""),
                    v.get("description", ""),
                    v.get("danger", ""),
                ),
            )

        # Build vuln_id lookup
        for v in vulns:
            cursor.execute("SELECT id FROM vulnerabilities WHERE name = ?", (v["name"],))
            row = cursor.fetchone()
            if row:
                vuln_ids[v["name"]] = row[0]

        # 6. options
        for o in opts:
            vid = vuln_ids.get(o["vuln_name"])
            if vid is None:
                continue
            cursor.execute(
                "INSERT INTO options(vuln_id, option_name, option_value) VALUES (?, ?, ?)",
                (vid, o["option_name"], o.get("option_value", "")),
            )

        # 7. modules (with mode→loud coupling matching C++ ModuD::add)
        for m in modules:
            mode_val = _to_bool(m.get("mode", False))
            loud_val = _to_bool(m.get("loud", False))
            if not mode_val:
                loud_val = 0
            cursor.execute(
                "INSERT INTO modules(name, path, platform, type, description, API, mode, loud, output) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (
                    m["name"],
                    m["path"],
                    m["platform"],
                    m.get("type", ""),
                    m.get("description", ""),
                    m.get("API", ""),
                    mode_val,
                    loud_val,
                    m.get("output", ""),
                ),
            )

        # 8. translations (after all content tables have IDs assigned)
        # Build known_ids lookup for all tables with name-based references
        known_ids = {}
        # categories
        known_ids["categories"] = {}
        c_cursor = conn.execute("SELECT id, name FROM categories")
        for row in c_cursor.fetchall():
            known_ids["categories"][row[1]] = row[0]
        # tools
        known_ids["tools"] = {}
        t_cursor = conn.execute("SELECT id, name FROM tools")
        for row in t_cursor.fetchall():
            known_ids["tools"][row[1]] = row[0]
        # vulnerabilities
        known_ids["vulnerabilities"] = {}
        v_cursor = conn.execute("SELECT id, name FROM vulnerabilities")
        for row in v_cursor.fetchall():
            known_ids["vulnerabilities"][row[1]] = row[0]
        # modules
        known_ids["modules"] = {}
        m_cursor = conn.execute("SELECT id, name FROM modules")
        for row in m_cursor.fetchall():
            known_ids["modules"][row[1]] = row[0]

        translations, tr_issues = _load_and_validate_translations(data_dir, known_ids)
        all_issues += tr_issues

        for tr in translations:
            tbl = tr["table_name"]
            row_name = tr["row_name"]
            row_id = known_ids.get(tbl, {}).get(row_name)
            if row_id is None:
                continue  # already reported as validation issue
            cursor.execute(
                "INSERT OR IGNORE INTO translations(table_name, row_id, column_name, lang, value) "
                "VALUES (?, ?, ?, ?, ?)",
                (
                    tbl,
                    row_id,
                    tr["column_name"],
                    tr["lang"],
                    tr["value"],
                ),
            )

        conn.commit()

    except sqlite3.Error as e:
        conn.rollback()
        raise RuntimeError(f"SQLite error during build: {e}") from e
    finally:
        conn.close()

    return db_path, all_issues


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
