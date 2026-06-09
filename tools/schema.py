"""
Tguide Database Schema Definitions

Matches the C++ CREATE TABLE statements in DatabaseManager.cpp
and the column validation in DBResolver.cpp::validateSchema().

All 7 tables must exist with exact column names and types.
"""

# ──────────────────────────────────────────────
# Table definitions
# ──────────────────────────────────────────────
# Each table spec is a dict with:
#   name        – SQL table name
#   columns     – list of (col_name, col_type, constraints) tuples
#   foreign_key – optional (local_col, ref_table, ref_col, on_delete)
#   extra       – optional extra SQL appended after columns

TABLES = [
    {
        "name": "vulnerabilities",
        "columns": [
            ("id",              "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("name",            "TEXT",    ""),
            ("metasploit_name", "TEXT",    ""),
            ("discovered_date", "TEXT",    ""),
            ("discoverer",      "TEXT",    ""),
            ("severity",        "TEXT",    ""),
            ("access",          "TEXT",    ""),
            ("platform",        "TEXT",    ""),
            ("service",         "TEXT",    ""),
            ("description",     "TEXT",    ""),
            ("danger",          "TEXT",    ""),
        ],
    },
    {
        "name": "options",
        "columns": [
            ("id",           "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("vuln_id",      "INTEGER", ""),
            ("option_name",  "TEXT",    ""),
            ("option_value", "TEXT",    ""),
        ],
        "foreign_key": ("vuln_id", "vulnerabilities", "id", "CASCADE"),
    },
    {
        "name": "modules",
        "columns": [
            ("id",          "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("name",        "TEXT",    ""),
            ("path",        "TEXT",    ""),
            ("platform",    "TEXT",    ""),
            ("type",        "TEXT",    ""),
            ("description", "TEXT",    ""),
            ("API",         "TEXT",    ""),
            ("mode",        "INTEGER", ""),
            ("loud",        "INTEGER", ""),
            ("output",      "TEXT",    ""),
        ],
    },
    {
        "name": "tools",
        "columns": [
            ("id",          "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("name",        "TEXT",    ""),
            ("category",    "TEXT",    ""),
            ("short_desc",  "TEXT",    ""),
            ("description", "TEXT",    ""),
            ("flags_all",   "TEXT",    ""),
        ],
    },
    {
        "name": "tool_flags",
        "columns": [
            ("id",          "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("tool_id",     "INTEGER", ""),
            ("name",        "TEXT",    ""),
            ("description", "TEXT",    ""),
            ("loud",        "TEXT",    ""),
            ("root",        "INTEGER", ""),
            ("protocols",   "TEXT",    ""),
        ],
        "foreign_key": ("tool_id", "tools", "id", "CASCADE"),
    },
    {
        "name": "templates",
        "columns": [
            ("id",            "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("tool_id",       "INTEGER", ""),
            ("template_name", "TEXT",    ""),
            ("description",   "TEXT",    ""),
            ("root",          "INTEGER", ""),
            ("protocols",     "TEXT",    ""),
            ("flag",          "INTEGER", ""),
        ],
        "foreign_key": ("tool_id", "tools", "id", "CASCADE"),
    },
    {
        "name": "categories",
        "columns": [
            ("id",             "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("name",           "TEXT",    ""),
            ("display_order",  "INTEGER", ""),
            ("description",    "TEXT",    ""),
        ],
    },
    {
        "name": "translations",
        "columns": [
            ("id",          "INTEGER", "PRIMARY KEY AUTOINCREMENT"),
            ("table_name",  "TEXT",    "NOT NULL"),
            ("row_id",      "INTEGER", "NOT NULL"),
            ("column_name", "TEXT",    "NOT NULL"),
            ("lang",        "TEXT",    "NOT NULL"),
            ("value",       "TEXT",    "NOT NULL"),
        ],
        "extra": "UNIQUE(table_name, row_id, column_name, lang)",
    },
]

# Order for insertion — respects foreign-key dependencies
INSERTION_ORDER = [
    "categories",       # no dependencies
    "tools",            # no dependencies
    "tool_flags",       # depends on tools
    "templates",        # depends on tools
    "vulnerabilities",  # no dependencies
    "options",          # depends on vulnerabilities
    "modules",          # no dependencies
    "translations",     # depends on all content tables
]


def get_all_tables():
    """Return the ordered list of table definitions."""
    return TABLES


def get_table_names():
    """Return list of all table names in definition order."""
    return [t["name"] for t in TABLES]


def get_insertion_order():
    """Return table names in foreign-key-safe insertion order."""
    return INSERTION_ORDER


def get_create_sql(table_def):
    """Generate a CREATE TABLE IF NOT EXISTS statement for one table."""
    col_defs = []
    for name, typ, constraints in table_def["columns"]:
        col = f"    {name} {typ}"
        if constraints:
            col += f" {constraints}"
        col_defs.append(col)

    col_text = ",\n".join(col_defs)
    fk = table_def.get("foreign_key")
    extra = table_def.get("extra", "")

    clauses = [col_text]
    if fk:
        local_col, ref_table, ref_col, on_delete = fk
        fk_text = (
            f"    FOREIGN KEY({local_col}) "
            f"REFERENCES {ref_table}({ref_col}) ON DELETE {on_delete}"
        )
        clauses.append(fk_text)
    if extra:
        clauses.append(f"    {extra}")

    return (
        f"CREATE TABLE IF NOT EXISTS {table_def['name']} (\n"
        + f",\n".join(clauses)
        + "\n"
        + ");"
    )


def create_tables(cursor):
    """Create all 7 tables using cursor.execute()."""
    for tdef in TABLES:
        cursor.execute(get_create_sql(tdef))
