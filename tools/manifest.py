"""
Tguide Manifest Generator

Generates and validates signed_manifest.json for tguide.db.

The manifest format must match what DBResolver::fetchManifest() expects:
  - "version": semver string (e.g. "1.0.0")
  - "db_hash": 64-char lowercase hex SHA-256 of the DB file
  - "db_url":  HTTPS URL pointing to the release asset
"""

import hashlib
import json
import os


def _sha256_file(filepath):
    """Compute SHA-256 hex digest of a file."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while True:
            chunk = f.read(65536)  # 64 KB buffer
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def generate_manifest(
    db_path,
    version="1.0.0",
    output_path="data/signed_manifest.json",
    db_url="https://github.com/voidoxin/Tguide/releases/latest/download/tguide.db",
):
    """
    Generate a signed_manifest.json for a built database.

    Args:
        db_path:     Path to the SQLite database file.
        version:     Version string (default "1.0.0").
        output_path: Where to write the manifest JSON.
        db_url:      Download URL for the database.

    Returns:
        True on success, False on error.
    """
    if not os.path.isfile(db_path):
        print(f"[ERROR] Database file not found: {db_path}")
        return False

    # Check version isn't empty
    if not version:
        print("[ERROR] Version string must not be empty.")
        return False

    # Validate db_url starts with https://
    if not db_url.startswith("https://"):
        print("[ERROR] db_url must start with https://")
        return False

    # Compute hash
    db_hash = _sha256_file(db_path)

    manifest = {
        "version": version,
        "db_hash": db_hash,
        "db_url": db_url,
    }

    # Ensure output directory exists
    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"[+] Manifest written to: {output_path}")
    print(f"    version  : {version}")
    print(f"    db_hash  : {db_hash}")
    print(f"    db_url   : {db_url}")
    return True


def validate_manifest(manifest_path, db_path):
    """
    Validate that a manifest file matches the given database.

    Args:
        manifest_path: Path to signed_manifest.json.
        db_path:       Path to the database file to verify.

    Returns:
        (is_valid, message) tuple.
    """
    if not os.path.isfile(manifest_path):
        return False, f"Manifest file not found: {manifest_path}"

    if not os.path.isfile(db_path):
        return False, f"Database file not found: {db_path}"

    try:
        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        return False, f"Failed to read manifest: {e}"

    # Check required fields
    for key in ("version", "db_hash", "db_url"):
        if key not in manifest:
            return False, f"Manifest missing required field: '{key}'"

    version = manifest["version"]
    db_hash = manifest["db_hash"]
    db_url = manifest["db_url"]

    if not version:
        return False, "Manifest has empty 'version' field."

    if len(db_hash) != 64:
        return False, (
            f"Manifest db_hash has wrong length: "
            f"got {len(db_hash)}, expected 64"
        )

    if not db_url.startswith("https://"):
        return False, "Manifest db_url must start with https://"

    # Compute actual hash
    actual_hash = _sha256_file(db_path)

    if actual_hash != db_hash:
        return False, (
            f"Hash mismatch!\n"
            f"  Manifest: {db_hash}\n"
            f"  Actual:   {actual_hash}"
        )

    return True, f"Manifest is valid (version {version}, hash matches)"
