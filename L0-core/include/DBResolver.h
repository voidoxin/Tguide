#pragma once
#include <string>
#include <unordered_map>

class DBResolver {
public:
    static DBResolver& instance();

    // Resolve a config path to a usable database path.
    // Caches results so repeated calls with the same path are O(1).
    std::string resolve(const std::string& configPath);

    // Returns true if a fatal error occurred during the last resolve.
    bool fatal() const { return fatal_; }

#ifndef NDEBUG
    // Reset all internal state — for test isolation.
    void resetForTesting();
#endif

private:
    DBResolver() = default;

    bool fatal_ = false;
    bool cacheValidated_ = false;
    std::unordered_map<std::string, std::string> resolvedCache_;

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
                                    const std::string& hash,
                                    bool officialHashSet);
    bool downloadDB(const std::string& url, const std::string& destPath);
};
