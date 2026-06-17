#pragma once
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

/*
 * DBCacheManager
 *
 * Manages the .db_cache file which tracks:
 *   - The currently active database hash
 *   - The last-seen manifest version
 *   - A rolling history of the last 3 accessed databases
 * This file is managed exclusively by the application.
 * Manual modification will corrupt integrity verification.
 *
 * Part of the Tools Guide project — open source.
 */

class DBCacheManager {
public:
    /*
     * Represents a single database access record stored in history.
     */
    struct DBRecord {
        std::string path;
        std::string hash;
    };

    // Singleton access — used by current call sites
    static DBCacheManager& instance();

    // Lifecycle
    /*
     * Initializes the cache system.
     * Creates the cache file and directories if they do not exist.
     * Must be called before any DB class is constructed.
     *
     * @param cachePath  Absolute path to the .db_cache file.
     */
    void init(const std::string& cachePath);

    /*
     * Loads the cache state from disk into memory.
     *
     * @param cachePath  Absolute path to the .db_cache file.
     * @return           True if loaded successfully, false otherwise.
     */
    bool load(const std::string& cachePath);

#ifndef NDEBUG
    /*
     * Reset all internal state — for test isolation.
     */
    void resetForTesting();
#endif

    // Persistence
    /*
     * Persists the current in-memory cache state to disk.
     * Uses the path established by init() or load().
     *
     * @return  True if saved successfully, false otherwise.
     */
    bool save();

    /*
     * Persists the current in-memory cache state to a specific path.
     *
     * @param cachePath  Absolute path to the .db_cache file.
     * @return           True if saved successfully, false otherwise.
     */
    bool save(const std::string& cachePath);

    // History
    /*
     * Records a database access in the rolling history.
     * Consecutive duplicate entries are deduplicated.
     * History is capped at 3 records — oldest entry is evicted when exceeded.
     *
     * @param path  Path to the accessed database file.
     * @param hash  SHA-256 hash of the accessed database file.
     */
    void recordAccess(const std::string& path, const std::string& hash);

    /*
     * Returns the full access history (up to 3 records).
     * Returns by value — safe to store across calls.
     */
    std::vector<DBRecord> getHistory();

    // Current hash
    /*
     * Updates the hash of the currently active database in the cache.
     * Should be called whenever the active database changes.
     *
     * @param hash  SHA-256 hash of the newly active database.
     */
    void setCurrentHash(const std::string& hash);

    /*
     * Returns the SHA-256 hash of the currently active database.
     * Returns by value — safe to store across calls.
     */
    std::string getCurrentHash();

    /*
     * Records the version string from the last successful manifest fetch.
     *
     * @param version  The version string from signed_manifest.json.
     */
    void setLastSeenVersion(const std::string& version);

    /*
     * Returns the version string from the last successful manifest fetch.
     * Returns empty string if no version has been recorded yet.
     */
    std::string getLastSeenVersion();

    /*
     * Stores the SHA-256 hash of the database backup file (.bak).
     * Empty string means no backup is available.
     */
    void setBackupHash(const std::string& hash);

    /*
     * Returns the SHA-256 hash of the database backup file (.bak).
     * Returns empty string if no backup has been recorded.
     */
    std::string getBackupHash();

    /*
     * Returns true if a backup hash has been recorded (backup exists).
     */
    bool hasBackup();

    /*
     * Clears the backup hash — called when backup is deleted or invalidated.
     */
    void clearBackup();

private:
    DBCacheManager() = default;

    json        cache_;
    std::string cachePath_;

    static constexpr int MAX_HISTORY_ = 3;
    json buildDefault();
    void repairSchema();
};
