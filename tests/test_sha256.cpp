#include "doctest.h"
#include "sha256.h"
#include "fixtures.h"

TEST_CASE("SHA256::hash(string) — empty string") {
    auto result = SHA256::hash("");
    CHECK(result == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_CASE("SHA256::hash(string) — known vector 'abc'") {
    auto result = SHA256::hash("abc");
    CHECK(result == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256::hash(string) — known vector 'message digest'") {
    auto result = SHA256::hash("message digest");
    CHECK(result == "f7846f55cf23e14eebeab5b4e1550cad5b509e3348fbc4efa3a1413d393cb650");
}

TEST_CASE("SHA256::hash(buffer) — matches string overload") {
    std::string input = "hello world";
    auto from_string = SHA256::hash(input);
    auto from_buffer = SHA256::hash(
        reinterpret_cast<const uint8_t*>(input.data()), input.size());
    CHECK(from_string == from_buffer);
}

TEST_CASE("SHA256::hash(buffer) — known vector") {
    const uint8_t data[] = {'a', 'b', 'c'};
    auto result = SHA256::hash(data, 3);
    CHECK(result == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256::hashFile — non-existent file returns empty") {
    auto result = SHA256::hashFile("/nonexistent/path/file.bin");
    CHECK(result == "");
}

TEST_CASE("SHA256::hashFile — existing file matches string hash") {
    test_fixtures::TempDirectory dir;
    std::string content = "file content test";
    auto filePath = test_fixtures::createTempFile(dir, "test.bin", content);

    auto fileHash = SHA256::hashFile(filePath);
    auto directHash = SHA256::hash(content);
    CHECK(fileHash == directHash);
}

TEST_CASE("SHA256::hashFile — empty file") {
    test_fixtures::TempDirectory dir;
    auto filePath = test_fixtures::createTempFile(dir, "empty.bin", "");

    auto fileHash = SHA256::hashFile(filePath);
    CHECK(fileHash == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
