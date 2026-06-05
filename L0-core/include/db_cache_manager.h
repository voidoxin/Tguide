#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../../libs/json.hpp"

using json = nlohmann::json;

/*
 * DBCacheManager
 *
 * Manages the .db_cache file which tracks:
 *   - The official database signature for integrity verification
 *   - The currently active database hash
 *   - A rolling history of the last 3 accessed databases                                            *
 * This file is managed exclusively by the application.
 * Manual modification will corrupt integrity verification.
 *
 * Part of the Tools Guide project — open source.
 */

// =============================================================
// CONFIGURE BEFORE RELEASE
// =============================================================

/*
 * SHA-256 hash of the official database shipped with this tool.
 * Set this value after generating the first official release of tguide.db.
 * Leave empty during development.
 */
static constexpr const char* DB_OFFICIAL_HASH = "";

/*
 * Direct download URL for the official database hosted on GitHub.
 * Used as a fallback when no valid local database is found.
 * Example: "https://raw.githubusercontent.com/user/tguide/main/data/tguide.db"
 */
static constexpr const char* DB_DOWNLOAD_URL = "";

// =============================================================

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

    /*
     * Searches the access history for any record whose hash matches
     * the official database signature, regardless of file path.
     * Skips records pointing to files that no longer exist on disk.
     *
     * @return  std::optional<DBRecord> containing the match, or std::nullopt.
     */
    std::optional<DBRecord> findOfficialInHistory();

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
     * Compares the currently active database hash against
     * the official database hash defined in DB_OFFICIAL_HASH.
     *
     * @return  True if the active database matches the official release.
     *          False if it differs, or if DB_OFFICIAL_HASH is not set.
     */
    bool isCurrentOfficial();

private:
    DBCacheManager() = default;

    json        cache_;
    std::string cachePath_;

    static constexpr int MAX_HISTORY_ = 3;
    json buildDefault();
    void repairSchema();
};
