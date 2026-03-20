#pragma once
#include <string>
#include <cstdint>

/*
 * SHA-256 implementation — no external dependencies
 * Produces a hex string digest of any file or data buffer
 * Compliant with FIPS 180-4 specification
 */

namespace SHA256 {

    /*
     * Compute SHA-256 hash of a raw data buffer
     * @param data   pointer to input bytes
     * @param length number of bytes to hash
     * @return       64-character lowercase hex string
     */
    std::string hash(const uint8_t* data, size_t length);

    /*
     * Compute SHA-256 hash of a string
     * @param input  input string
     * @return       64-character lowercase hex string
     */
    std::string hash(const std::string& input);

    /*
     * Compute SHA-256 hash of a file on disk
     * Reads the file in chunks — safe for large files
     * @param filepath  absolute or relative path to file
     * @return          64-character lowercase hex string, or "" on failure
     */
    std::string hashFile(const std::string& filepath);

} // namespace SHA256