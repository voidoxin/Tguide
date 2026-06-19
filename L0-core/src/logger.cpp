/*
 *  tguide — logger.cpp
 *  written by voidoxin
 */

#include "logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

// ==================== singleton ====================

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    // nothing to clean up
}

// ==================== lifecycle ====================

void Logger::init(const std::string& logPath) {
    m_logPath = logPath;

    // ensure parent directory exists
    fs::path p(logPath);
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);

    // create file if it doesn't exist
    {
        std::ofstream file(logPath, std::ios::app);
        // file created / opened
    }

    // scan existing entries for the highest boot ID
    int maxBootId = 0;
    auto entries = readAllEntries();
    for (const auto& e : entries) {
        if (e.bootId > maxBootId)
            maxBootId = e.bootId;
    }
    m_currentBootId = maxBootId + 1;

    // write marker entry
    LogEntry marker;
    marker.timestamp = std::chrono::system_clock::now();
    marker.level     = INFO;
    marker.bootId    = m_currentBootId;
    marker.message   = "Logger initialized";
    writeEntry(marker);

    m_initialized = true;

    // apply retention policy
    applyRetentionPolicy();
}

bool Logger::isInitialized() const {
    return m_initialized;
}

// ==================== logging ====================

void Logger::log(Level level, const std::string& message) {
    if (!m_initialized) return;

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level     = level;
    entry.bootId    = m_currentBootId;
    entry.message   = message;

    writeEntry(entry);
}

void Logger::info(const std::string& message)    { log(INFO,    message); }
void Logger::warning(const std::string& message) { log(WARNING, message); }
void Logger::error(const std::string& message)   { log(ERROR,   message); }
void Logger::debug(const std::string& message)   { log(DEBUG,   message); }

// ==================== query API ====================

std::vector<LogEntry> Logger::allEntries() const {
    return readAllEntries();
}

std::vector<LogEntry> Logger::entriesForBoot(int bootId) const {
    auto all = readAllEntries();
    std::vector<LogEntry> result;
    for (const auto& e : all) {
        if (e.bootId == bootId)
            result.push_back(e);
    }
    return result;
}

std::vector<LogEntry> Logger::entriesForDate(const std::string& date) const {
    auto all = readAllEntries();
    std::vector<LogEntry> result;
    for (const auto& e : all) {
        std::string ts = formatTimestamp(e.timestamp);
        // "YYYY-MM-DD HH:MM:SS" → extract first 10 chars
        if (ts.size() >= 10 && ts.substr(0, 10) == date)
            result.push_back(e);
    }
    return result;
}

int Logger::currentBootId() const {
    return m_currentBootId;
}

// ==================== retention ====================

void Logger::applyRetentionPolicy() {
    if (m_logPath.empty()) return;

    auto entries = readAllEntries();
    if (entries.empty()) return;

    auto now = std::chrono::system_clock::now();

    // helper: compute serialized text size for a vector of entries
    auto serializedSize = [&](const std::vector<LogEntry>& vec) -> size_t {
        std::ostringstream oss;
        for (const auto& e : vec) {
            std::string msg = e.message;
            std::string clean;
            for (char c : msg) {
                clean += (c == '\n') ? ' ' : c;
            }
            oss << formatTimestamp(e.timestamp)
                << " [" << levelToString(e.level) << "]"
                << " [boot=" << e.bootId << "] "
                << clean << "\n";
        }
        return oss.str().size();
    };

    // ── Step 1 (time-based) ──────────────────────────────────────────
    // Remove entries older than 6 months
    auto sixMonths = std::chrono::hours(24 * 180);
    auto cutoff6   = now - sixMonths;

    std::vector<LogEntry> kept;
    for (const auto& e : entries) {
        if (e.timestamp >= cutoff6)
            kept.push_back(e);
    }

    // ── Step 2 (size-based, 5 months) ────────────────────────────────
    size_t size = serializedSize(kept);
    if (size > m_maxLogSize) {
        auto fiveMonths = std::chrono::hours(24 * 150);
        auto cutoff5    = now - fiveMonths;

        std::vector<LogEntry> kept2;
        for (const auto& e : kept) {
            if (e.timestamp >= cutoff5)
                kept2.push_back(e);
        }
        kept = kept2;
        size = serializedSize(kept);
    }

    // ── Step 3 (progressive, 3 months) ───────────────────────────────
    if (size > m_maxLogSize) {
        auto threeMonths = std::chrono::hours(24 * 90);
        auto cutoff3     = now - threeMonths;

        std::vector<LogEntry> kept3;
        for (const auto& e : kept) {
            if (e.timestamp >= cutoff3)
                kept3.push_back(e);
        }
        kept = kept3;
    }

    // ── write back, overwriting the file ─────────────────────────────
    std::ofstream out(m_logPath, std::ios::trunc);
    if (!out.is_open()) return;

    for (const auto& e : kept) {
        std::string msg = e.message;
        std::string clean;
        for (char c : msg) {
            clean += (c == '\n') ? ' ' : c;
        }
        out << formatTimestamp(e.timestamp)
            << " [" << levelToString(e.level) << "]"
            << " [boot=" << e.bootId << "] "
            << clean << "\n";
    }
}

void Logger::setMaxLogSizeForTesting(size_t bytes) {
    m_maxLogSize = bytes;
}

// ==================== file I/O ====================

void Logger::writeEntry(const LogEntry& entry) {
    std::ofstream file(m_logPath, std::ios::app);
    if (!file.is_open()) return;

    std::string msg = entry.message;
    std::string clean;
    for (char c : msg) {
        clean += (c == '\n') ? ' ' : c;
    }

    file << formatTimestamp(entry.timestamp)
         << " [" << levelToString(entry.level) << "]"
         << " [boot=" << entry.bootId << "] "
         << clean << "\n";
}

std::vector<LogEntry> Logger::readAllEntries() const {
    std::vector<LogEntry> entries;
    std::ifstream file(m_logPath);
    if (!file.is_open()) return entries;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // pattern: YYYY-MM-DD HH:MM:SS [LEVEL] [boot=N] message
        static const std::regex pattern(
            R"(^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) )"
            R"(\[(DEBUG|INFO|WARNING|ERROR)\] )"
            R"(\[boot=(\d+)\] (.*)$)"
        );
        std::smatch match;
        if (!std::regex_match(line, match, pattern))
            continue;   // skip malformed lines

        LogEntry entry;
        entry.timestamp = parseTimestamp(match[1].str());
        entry.level     = levelFromString(match[2].str());
        entry.bootId    = std::stoi(match[3].str());
        entry.message   = match[4].str();

        entries.push_back(entry);
    }

    return entries;
}

// ==================== formatting / parsing ====================

std::string Logger::levelToString(Level level) const {
    switch (level) {
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARNING";
        case ERROR:   return "ERROR";
        default:      return "UNKNOWN";
    }
}

Logger::Level Logger::levelFromString(const std::string& s) const {
    if (s == "DEBUG")   return DEBUG;
    if (s == "INFO")    return INFO;
    if (s == "WARNING") return WARNING;
    if (s == "ERROR")   return ERROR;
    return INFO; // fallback
}

std::string Logger::formatTimestamp(
    const std::chrono::system_clock::time_point& tp) const
{
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::chrono::system_clock::time_point Logger::parseTimestamp(
    const std::string& s) const
{
    std::tm tm = {};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto t = mktime(&tm);                     // interpret as local time
    return std::chrono::system_clock::from_time_t(t);
}
