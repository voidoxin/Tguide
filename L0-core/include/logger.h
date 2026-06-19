/*
 *  tguide — logger.h
 *  internal logging system (singleton)
 *
 *  written by voidoxin
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>

// ==================== LogLevel ====================

struct LogLevel {
    enum Level { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };
};

// ==================== LogEntry ====================

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string message;
    LogLevel::Level level;
    int bootId;
};

// ==================== Logger ====================

/*
 * Singleton internal logger with:
 *   - Severity levels (DEBUG, INFO, WARNING, ERROR)
 *   - Auto-incrementing boot session tracking
 *   - Log rotation with retention policy (time + size based)
 *   - Query API for future log viewer (STEP-25)
 *
 * Single-threaded project — no mutex needed.
 * Writes to a plaintext file only — no cout/cerr.
 */
class Logger {
public:
    using Level = LogLevel::Level;
    static constexpr Level DEBUG   = LogLevel::DEBUG;
    static constexpr Level INFO    = LogLevel::INFO;
    static constexpr Level WARNING = LogLevel::WARNING;
    static constexpr Level ERROR   = LogLevel::ERROR;

    static Logger& instance();

    // Initialize with absolute path to the log file.
    // Creates the file and parent directories if they don't exist.
    // Scans existing file for highest boot ID, increments by 1,
    // writes a marker entry, and applies retention policy.
    void init(const std::string& logPath);

    bool isInitialized() const;

    // ── core logging method ──────────────────────────────────────────────
    void log(Level level, const std::string& message);

    // ── convenience wrappers ────────────────────────────────────────────
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);

    // ── query API (consumed by STEP-25) ─────────────────────────────────
    std::vector<LogEntry> allEntries() const;
    std::vector<LogEntry> entriesForBoot(int bootId) const;
    std::vector<LogEntry> entriesForDate(const std::string& date) const;
    int currentBootId() const;

    // ── retention policy ────────────────────────────────────────────────
    // Applies on init() and can be called manually.
    void applyRetentionPolicy();

    // ── test helper ─────────────────────────────────────────────────────
    // Override the max log size so tests can trigger pruning without
    // writing millions of entries.
    void setMaxLogSizeForTesting(size_t bytes);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // ── internal helpers ────────────────────────────────────────────────
    void writeEntry(const LogEntry& entry);
    std::vector<LogEntry> readAllEntries() const;
    std::string levelToString(Level level) const;
    Level levelFromString(const std::string& s) const;
    std::string formatTimestamp(
        const std::chrono::system_clock::time_point& tp) const;
    std::chrono::system_clock::time_point parseTimestamp(
        const std::string& s) const;

    std::string m_logPath;
    int m_currentBootId = 0;
    bool m_initialized = false;

    // 10 MB max before pruning kicks in
    size_t m_maxLogSize = 10 * 1024 * 1024;
};
