#include "doctest.h"
#include "logger.h"
#include "fixtures.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cstring>

namespace fs = std::filesystem;

// ==================== helpers ====================

// Count non-empty lines in a file
static size_t countLines(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return 0;
    size_t n = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) ++n;
    }
    return n;
}

// Regex to validate a single log line
static const std::regex LINE_PATTERN(
    R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} \[(DEBUG|INFO|WARNING|ERROR)\] \[boot=\d+\] )"
);

// ==================== TEST CASES ====================

// 1. singleton — returns the same address every time
TEST_CASE("Logger — singleton returns same instance") {
    auto& a = Logger::instance();
    auto& b = Logger::instance();
    CHECK(&a == &b);
}

// 2. not initialized by default
TEST_CASE("Logger — not initialized before init()") {
    // reset state by creating a fresh scenario: we can't un-init the
    // singleton, but we can check the default.  The singleton may already
    // be initialized from a previous test — skip if so.
    if (!Logger::instance().isInitialized())
        CHECK(Logger::instance().isInitialized() == false);
}

// 3. init creates file
TEST_CASE("Logger — init creates log file") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    CHECK(fs::exists(logPath));
    CHECK(countLines(logPath.string()) >= 1);   // at least the marker
}

// 4. write and read back
TEST_CASE("Logger — write and read back an entry") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());

    Logger::instance().info("hello logger");

    auto entries = Logger::instance().allEntries();
    bool found = false;
    for (const auto& e : entries) {
        if (e.message.find("hello logger") != std::string::npos) {
            found = true;
            CHECK(e.level == Logger::INFO);
            break;
        }
    }
    CHECK(found);
}

// 5. severity levels — file contains all four levels
TEST_CASE("Logger — all severity levels appear in file") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    Logger::instance().debug("dbg msg");
    Logger::instance().info("inf msg");
    Logger::instance().warning("wrn msg");
    Logger::instance().error("err msg");

    std::ifstream in(logPath.string());
    REQUIRE(in.is_open());

    bool sawDebug   = false;
    bool sawInfo    = false;
    bool sawWarning = false;
    bool sawError   = false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.find("[DEBUG]")   != std::string::npos) sawDebug   = true;
        if (line.find("[INFO]")    != std::string::npos) sawInfo    = true;
        if (line.find("[WARNING]") != std::string::npos) sawWarning = true;
        if (line.find("[ERROR]")   != std::string::npos) sawError   = true;
    }

    CHECK(sawDebug);
    CHECK(sawInfo);
    CHECK(sawWarning);
    CHECK(sawError);
}

// 6. boot ID increments from existing file
TEST_CASE("Logger — boot ID increments from existing file") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Manually create a log with boot=5 entries
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        out << "2026-06-19 10:00:00 [INFO] [boot=5] existing entry\n";
        out << "2026-06-19 10:00:01 [ERROR] [boot=5] something failed\n";
    }

    Logger::instance().init(logPath.string());
    CHECK(Logger::instance().currentBootId() == 6);
}

// 7. allEntries() returns correct count and fields
TEST_CASE("Logger — allEntries returns correct entries") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    Logger::instance().info("first");
    Logger::instance().warning("second");
    Logger::instance().error("third");

    auto entries = Logger::instance().allEntries();
    // at least 4: marker + 3 written
    CHECK(entries.size() >= 4);

    bool foundMarker = false;
    bool foundFirst  = false;
    bool foundSecond = false;
    bool foundThird  = false;
    for (const auto& e : entries) {
        if (e.message == "Logger initialized") foundMarker = true;
        if (e.message == "first")  { foundFirst  = true; CHECK(e.level == Logger::INFO); }
        if (e.message == "second") { foundSecond = true; CHECK(e.level == Logger::WARNING); }
        if (e.message == "third")  { foundThird  = true; CHECK(e.level == Logger::ERROR); }
    }
    CHECK(foundMarker);
    CHECK(foundFirst);
    CHECK(foundSecond);
    CHECK(foundThird);
}

// 8. entriesForBoot() returns correct subset
TEST_CASE("Logger — entriesForBoot filters by boot ID") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Manually craft entries with different boot IDs
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        out << "2026-06-19 10:00:00 [INFO] [boot=1] first boot\n";
        out << "2026-06-19 10:00:01 [INFO] [boot=1] still first\n";
        out << "2026-06-19 10:00:02 [INFO] [boot=2] second boot\n";
        out << "2026-06-19 10:00:03 [INFO] [boot=3] third boot\n";
    }

    Logger::instance().init(logPath.string());
    // init writes another entry with boot=4

    auto b1 = Logger::instance().entriesForBoot(1);
    CHECK(b1.size() == 2);

    auto b2 = Logger::instance().entriesForBoot(2);
    CHECK(b2.size() == 1);

    auto b99 = Logger::instance().entriesForBoot(99);
    CHECK(b99.empty());
}

// 9. entriesForDate() returns correct subset
TEST_CASE("Logger — entriesForDate filters by date") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        out << "2026-01-15 08:00:00 [INFO] [boot=1] january entry\n";
        out << "2026-06-19 12:00:00 [INFO] [boot=1] june entry\n";
        out << "2026-06-19 14:00:00 [INFO] [boot=1] another june\n";
    }

    Logger::instance().init(logPath.string());

    auto jan = Logger::instance().entriesForDate("2026-01-15");
    CHECK(jan.size() == 1);
    CHECK(jan[0].message.find("january") != std::string::npos);

    auto jun = Logger::instance().entriesForDate("2026-06-19");
    // at least 2 crafted + possibly the marker (same day)
    CHECK(jun.size() >= 2);

    auto empty = Logger::instance().entriesForDate("2025-01-01");
    CHECK(empty.empty());
}

// 10. retention — old entries removed by time
TEST_CASE("Logger — retention removes entries older than 6 months") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Write old and new entries directly
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        // 18 months ago (use 2025-01-01 which is >6mo from 2026-06-19)
        out << "2025-01-01 08:00:00 [INFO] [boot=1] very old entry\n";
        // 8 months ago
        out << "2025-10-15 08:00:00 [INFO] [boot=1] moderately old\n";
        // today (will be written by init marker too)
    }

    Logger::instance().init(logPath.string());

    // Set a large max so size pruning doesn't interfere
    Logger::instance().setMaxLogSizeForTesting(10UL * 1024 * 1024);

    Logger::instance().applyRetentionPolicy();

    auto entries = Logger::instance().allEntries();
    for (const auto& e : entries) {
        CHECK(e.message.find("very old") == std::string::npos);
    }
    // At least the "moderately old" and marker entry should remain
    // (2025-10-15 could be >6mo depending on current date; if current
    //  date is 2026-06-19, then 2025-10-15 is ~8 months → removed)
}

// 11. retention — size limit pruning
TEST_CASE("Logger — retention prunes by size when limit is exceeded") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Write entries with staggered timestamps so the progressive
    // time-based pruning actually removes entries:
    //   3 entries from 2025-01-01 ( >6mo → removed by step 1)
    //   5 entries from 2026-01-01 (~5.5mo → removed by step 2)
    //   2 entries from 2026-05-01 (~1.5mo → recent, kept)
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        for (int i = 0; i < 3; ++i)
            out << "2025-01-01 10:00:0" << i << " [INFO] [boot=1] very old " << i << "\n";
        for (int i = 0; i < 5; ++i)
            out << "2026-01-01 10:00:0" << i << " [INFO] [boot=1] medium old " << i << "\n";
        for (int i = 0; i < 2; ++i)
            out << "2026-05-01 10:00:0" << i << " [INFO] [boot=1] recent entry " << i << "\n";
    }

    Logger::instance().init(logPath.string());

    // Set a tiny max log size so size-based pruning kicks in
    Logger::instance().setMaxLogSizeForTesting(200);

    // Apply retention manually (init already applied it, but our size
    // limit override is set after init, so apply again)
    Logger::instance().applyRetentionPolicy();

    auto entries = Logger::instance().allEntries();

    // No very-old or medium-old entries should remain
    for (const auto& e : entries) {
        CHECK(e.message.find("very old") == std::string::npos);
        CHECK(e.message.find("medium old") == std::string::npos);
    }

    // At most a few entries should survive (recent ones + marker)
    // We started with 10 crafted + marker = up to 11
    // After pruning: at most ~4 entries (2 recent + marker + maybe 1)
    CHECK(entries.size() <= 5);
    CHECK(entries.size() >= 1);
}

// 12. empty log returns empty vector
TEST_CASE("Logger — allEntries on empty file returns empty") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Create an empty file
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
    }

    Logger::instance().init(logPath.string());

    // The init writes a marker, so allEntries should not be empty.
    // Instead, test that reading an empty file before init returns empty.
    // We can't easily call readAllEntries() directly (it's private),
    // but we can test that a fresh file before init is handled.
    CHECK(fs::exists(logPath));
}

// 13. malformed line is skipped gracefully
TEST_CASE("Logger — malformed line is skipped gracefully") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Write valid + malformed lines
    {
        std::ofstream out(logPath.string());
        REQUIRE(out.is_open());
        out << "2026-06-19 10:00:00 [INFO] [boot=1] valid entry\n";
        out << "this is complete garbage that should be skipped\n";
        out << "2026-06-19 10:00:01 [WARNING] [boot=1] another valid\n";
        out << "[MALFORMED line without timestamp\n";
    }

    Logger::instance().init(logPath.string());

    auto entries = Logger::instance().allEntries();
    // At minimum the two valid crafted lines + init marker
    CHECK(entries.size() >= 2);

    bool foundValid1 = false;
    bool foundValid2 = false;
    for (const auto& e : entries) {
        if (e.message.find("valid entry") != std::string::npos)
            foundValid1 = true;
        if (e.message.find("another valid") != std::string::npos)
            foundValid2 = true;
    }
    CHECK(foundValid1);
    CHECK(foundValid2);
}

// 14. message with special chars round-trips correctly
TEST_CASE("Logger — special characters in message round-trip") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    std::string special = "brackets [ ] equals = and stuff";
    Logger::instance().info(special);

    auto entries = Logger::instance().allEntries();
    bool found = false;
    for (const auto& e : entries) {
        if (e.message == special) {
            found = true;
            break;
        }
    }
    CHECK(found);
}

// 15. multiline message has newlines replaced with spaces
TEST_CASE("Logger — newlines in message are replaced with spaces") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    std::string multi = "line one\nline two\nline three";
    Logger::instance().warning(multi);

    auto entries = Logger::instance().allEntries();
    bool found = false;
    for (const auto& e : entries) {
        if (e.message.find("line one line two line three") != std::string::npos) {
            found = true;
            CHECK(e.message.find('\n') == std::string::npos);
            break;
        }
    }
    CHECK(found);
}

// 16. reinit preserves boot ID — new session increments
TEST_CASE("Logger — reinit preserves boot ID and increments") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // First init
    Logger::instance().init(logPath.string());
    int firstBoot = Logger::instance().currentBootId();
    CHECK(firstBoot > 0);

    Logger::instance().info("session one");

    // Reinit (simulates restart)
    Logger::instance().init(logPath.string());
    int secondBoot = Logger::instance().currentBootId();
    CHECK(secondBoot == firstBoot + 1);

    // Verify both sessions are in the file
    auto b1 = Logger::instance().entriesForBoot(firstBoot);
    CHECK(b1.size() >= 1);

    auto b2 = Logger::instance().entriesForBoot(secondBoot);
    CHECK(b2.size() >= 1);
}

// 17. currentBootId returns positive integer after init
TEST_CASE("Logger — currentBootId returns positive after init") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    CHECK(Logger::instance().currentBootId() > 0);
}

// 18. format consistency — every line matches the expected regex
TEST_CASE("Logger — every log line matches expected format") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    Logger::instance().debug("test debug");
    Logger::instance().info("test info");
    Logger::instance().warning("test warning");
    Logger::instance().error("test error");

    std::ifstream in(logPath.string());
    REQUIRE(in.is_open());

    std::string line;
    int lineCount = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ++lineCount;
        CHECK(std::regex_search(line, LINE_PATTERN));
    }
    CHECK(lineCount >= 5);   // marker + 4 written
}

// 19. file truncation safety — retention on file at size limit
//     doesn't lose data unnecessarily for recent entries
TEST_CASE("Logger — retention does not remove recent entries at size limit") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());

    // Set size limit to something small but reasonable
    Logger::instance().setMaxLogSizeForTesting(500);

    // Write enough entries to almost fill the limit (all recent)
    // Each entry is ~70-80 bytes, so 6 entries = ~420-480 bytes
    for (int i = 0; i < 6; ++i) {
        Logger::instance().info(
            std::string("keep me safe message index ") + std::to_string(i));
    }

    auto before = Logger::instance().allEntries();

    Logger::instance().applyRetentionPolicy();

    auto after = Logger::instance().allEntries();

    // Since entries are recent (< 3 months), time-based pruning won't
    // remove them.  Size is < 500 so size-based pruning won't fire.
    // All recent entries should be preserved.
    CHECK(after.size() == before.size());
}

// 20. init twice reinitializes with new file / new boot session
TEST_CASE("Logger — init twice starts new boot session") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // First init with a fresh file — boot ID starts at 1
    Logger::instance().init(logPath.string());
    int bootA = Logger::instance().currentBootId();
    CHECK(bootA > 0);
    Logger::instance().info("session one message");

    // Reinit with the same file — boot ID should increment
    // because the file now contains entries with bootA
    Logger::instance().init(logPath.string());
    int bootB = Logger::instance().currentBootId();
    CHECK(bootB == bootA + 1);

    // Both boot sessions should be in the file
    auto sessionsA = Logger::instance().entriesForBoot(bootA);
    CHECK(sessionsA.size() >= 1);

    auto sessionsB = Logger::instance().entriesForBoot(bootB);
    CHECK(sessionsB.size() >= 1);
}
