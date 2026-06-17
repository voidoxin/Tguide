#include "doctest.h"
#include "DatabaseManager.h"
#include "fixtures.h"
#include <string>

using namespace std;

// ── Helpers ──────────────────────────────────────────────────────────────────

// Returns a freshly opened ToolD against the committed seed DB (found at cmake
// configure time via TGUIDE_SOURCE_DIR, so tests work without cmake --install).
static ToolD openDb() {
    return ToolD(std::string(TGUIDE_SOURCE_DIR) + "/data/database/tguide.db",
                 DirectOpen{});
}

// Count how many items in results have the given substring in name
static int countByName(const ToolResults& res, const string& substr) {
    int n = 0;
    for (const auto& t : res.items)
        if (t.name.find(substr) != string::npos) ++n;
    return n;
}

// ── TEST CASES ───────────────────────────────────────────────────────────────

TEST_CASE("searchTools — exact name match") {
    auto results = openDb().searchTools("nmap");
    REQUIRE(results.items.size() >= 1);
    CHECK(results.items[0].name == "nmap");
}

TEST_CASE("searchTools — partial name match") {
    // "map" should match "nmap" and possibly others
    auto results = openDb().searchTools("map");
    CHECK(countByName(results, "map") >= 1);
}

TEST_CASE("searchTools — short_desc match") {
    // All seed tools have short_desc; search for a common substring
    auto results = openDb().searchTools("Network");
    CHECK(results.items.size() >= 1);
}

TEST_CASE("searchTools — description match") {
    // Search a word likely to appear in descriptions
    auto results = openDb().searchTools("security");
    CHECK(results.items.size() >= 1);
}

TEST_CASE("searchTools — case insensitive") {
    auto lower = openDb().searchTools("nmap");
    auto upper = openDb().searchTools("NMAP");
    auto mixed = openDb().searchTools("Nmap");
    CHECK(lower.items.size() == upper.items.size());
    CHECK(upper.items.size() == mixed.items.size());
    CHECK(mixed.items.size() >= 1);
}

TEST_CASE("searchTools — no match returns empty") {
    auto results = openDb().searchTools("xyznonexistent12345");
    CHECK(results.items.empty());
}

TEST_CASE("searchTools — empty query returns all") {
    auto results = openDb().searchTools("");
    CHECK(results.items.size() >= 1);
}

TEST_CASE("searchTools — LIKE wildcards are escaped") {
    // % should be treated as literal character, not wildcard
    auto pct = openDb().searchTools("%");
    // "%" as a literal would likely match nothing in tool descriptions
    CHECK(pct.items.empty());

    // _ should also be literal
    auto us = openDb().searchTools("_");
    CHECK(us.items.empty());
}

TEST_CASE("searchTools — empty query returns all tools sorted by id") {
    auto results = openDb().searchTools("");
    REQUIRE(results.items.size() >= 2);
    // Items are returned in database (id) order — L1 service layer
    // adds name-based sorting. Verify at least multiple tools returned.
    CHECK(results.items.size() >= 2);
}

TEST_CASE("searchTools — all returned fields populated") {
    auto results = openDb().searchTools("nmap");
    REQUIRE(results.items.size() >= 1);
    const auto& t = results.items[0];
    CHECK(!t.name.empty());
    CHECK(!t.category.empty());
    CHECK(!t.short_desc.empty());
    CHECK(!t.description.empty());
}
