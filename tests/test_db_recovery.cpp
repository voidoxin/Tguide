#include "doctest.h"
#include "db_cache_manager.h"
#include "DBResolver.h"
#include "fixtures.h"
#include "sha256.h"
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <string>

// =============================================================
// DBCacheManager — backup hash round-trip
// =============================================================
TEST_CASE("DBCacheManager — backup hash round-trip") {
    DBCacheManager& mgr = DBCacheManager::instance();

#ifndef NDEBUG
    mgr.resetForTesting();
#endif

    // Start clean
    mgr.clearBackup();
    CHECK(mgr.getBackupHash() == "");
    CHECK(mgr.hasBackup() == false);

    // Set a backup hash
    mgr.setBackupHash("abcdef1234567890abcdef1234567890");
    CHECK(mgr.getBackupHash() == "abcdef1234567890abcdef1234567890");
    CHECK(mgr.hasBackup() == true);

    // Clear it
    mgr.clearBackup();
    CHECK(mgr.getBackupHash() == "");
    CHECK(mgr.hasBackup() == false);
}

// =============================================================
// DBCacheManager — default backup hash is empty
// =============================================================
TEST_CASE("DBCacheManager — default backup hash is empty") {
    DBCacheManager& mgr = DBCacheManager::instance();

#ifndef NDEBUG
    mgr.resetForTesting();
#endif

    CHECK(mgr.getBackupHash() == "");
    CHECK(mgr.hasBackup() == false);
}

// =============================================================
// DBResolver — tryRestoreFromBackup with missing file
// =============================================================
TEST_CASE("DBResolver — tryRestoreFromBackup with missing file") {
    test_fixtures::TempDirectory dir;

    std::string bakPath   = (dir.path / "nonexistent.bak").string();
    std::string destPath  = (dir.path / "restored.db").string();

    bool result = DBResolver::tryRestoreFromBackup(bakPath, destPath);
    CHECK(result == false);

    // Destination should not have been created
    CHECK(std::filesystem::exists(destPath) == false);
}

// =============================================================
// DBResolver — tryRestoreFromBackup with invalid file
// =============================================================
TEST_CASE("DBResolver — tryRestoreFromBackup with invalid file") {
    test_fixtures::TempDirectory dir;

    std::string bakPath  = test_fixtures::createTempFile(
        dir, "backup.bak", "not a database");
    std::string destPath = (dir.path / "restored.db").string();

    bool result = DBResolver::tryRestoreFromBackup(bakPath, destPath);
    CHECK(result == false);

    // Destination should not have been created
    CHECK(std::filesystem::exists(destPath) == false);
}

// =============================================================
// DBResolver — tryRestoreFromBackup hash mismatch
//
// Creates a valid SQLite database, records its hash, then
// replaces the backup with different content. The restore
// should fail because the hash of the on-disk backup no
// longer matches the recorded hash.
// =============================================================
TEST_CASE("DBResolver — tryRestoreFromBackup hash mismatch") {
    test_fixtures::TempDirectory dir;

    // ── Step 1: create a valid SQLite database file ─────────────
    std::string bakPath = (dir.path / "backup.bak").string();
    {
        sqlite3* db = nullptr;
        int rc = sqlite3_open(bakPath.c_str(), &db);
        REQUIRE(rc == SQLITE_OK);
        // Execute a trivial statement to ensure a non-empty file with a
        // proper SQLite header is written to disk.
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS _test_backup (id INT)",
                          nullptr, nullptr, &errMsg);
        REQUIRE(rc == SQLITE_OK);
        sqlite3_free(errMsg);
        sqlite3_close(db);
    }
    REQUIRE(std::filesystem::exists(bakPath));
    REQUIRE(std::filesystem::file_size(bakPath) > 0);

    // ── Step 2: record its hash in DBCacheManager ──────────────
    DBCacheManager& mgr = DBCacheManager::instance();
#ifndef NDEBUG
    mgr.resetForTesting();
#endif
    std::string originalHash = SHA256::hashFile(bakPath);
    REQUIRE(originalHash.empty() == false);
    mgr.setBackupHash(originalHash);
    CHECK(mgr.getBackupHash() == originalHash);
    CHECK(mgr.hasBackup() == true);

    // ── Step 3: replace backup with different content ──────────
    // Overwrite the file so its hash changes
    {
        std::ofstream out(bakPath, std::ios::binary | std::ios::trunc);
        out << "this is completely different content — not a database";
    }
    std::string newHash = SHA256::hashFile(bakPath);
    REQUIRE(newHash.empty() == false);
    REQUIRE(newHash != originalHash);   // must be different

    // ── Step 4: try to restore — should fail (hash mismatch) ──
    std::string destPath = (dir.path / "restored.db").string();
    bool result = DBResolver::tryRestoreFromBackup(bakPath, destPath);
    CHECK(result == false);

    // Destination should not have been created
    CHECK(std::filesystem::exists(destPath) == false);
}
