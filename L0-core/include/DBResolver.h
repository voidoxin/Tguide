#pragma once
#include <string>
#include <unordered_map>

class DBResolver {
public:
    /*
     * Data from the version manifest fetched from GitHub.
     */
    struct Manifest {
        std::string version;
        std::string db_hash;
        std::string db_url;
    };

    static DBResolver& instance();

    // Resolve a config path to a usable database path.
    // Caches results so repeated calls with the same path are O(1).
    std::string resolve(const std::string& configPath);

    // Returns true if a fatal error occurred during the last resolve.
    bool fatal() const { return fatal_; }

    // Fetch and parse the version manifest from GitHub.
    // Returns empty Manifest on failure (caller handles fallback).
    Manifest fetchManifest();

    // Returns the last successfully fetched manifest.
    Manifest getManifest() const { return manifest_; }

#ifndef NDEBUG
    // Reset all internal state — for test isolation.
    void resetForTesting();
#endif

    // Attempt to restore from a backup file. Returns true on success.
    static bool tryRestoreFromBackup(const std::string& bakPath,
                                     const std::string& destPath);

private:
    DBResolver() = default;

    bool fatal_ = false;
    bool cacheValidated_ = false;
    std::unordered_map<std::string, std::string> resolvedCache_;

    Manifest manifest_;
    bool manifestFetched_ = false;

    // Helper methods
    void invalidateCacheIfMissing();
    std::string cacheResult(const std::string& configPath,
                            const std::string& resolvedPath,
                            const std::string& hash);
    bool openAndValidate(const std::string& path, bool& isSQLite,
                         bool& schemaOk, bool& hasData,
                         std::string& hash);
    std::string copyDefaultToConfig(const std::string& configPath);
    std::string resolveHashMismatch(const std::string& configPath,
                                    const std::string& hash);
    bool downloadDB(const std::string& url, const std::string& destPath);

};
