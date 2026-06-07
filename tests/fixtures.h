#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <functional>
#include <ctime>
#include <vector>
#include "ErrorHandler.h"

namespace test_fixtures {

// ==================== TempDirectory ====================
// Creates a unique temporary directory, destroyed on scope exit.
struct TempDirectory {
    std::filesystem::path path;

    TempDirectory()
        : path(std::filesystem::temp_directory_path()
               / ("tguide_test_" + std::to_string(std::time(nullptr)))) {
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
};

// ==================== TempFile ====================
// Creates a temp file with given content in a TempDirectory.
inline std::string createTempFile(TempDirectory& dir,
                                  const std::string& filename,
                                  const std::string& content) {
    auto fullPath = dir.path / filename;
    std::ofstream out(fullPath);
    out << content;
    out.close();
    return fullPath.string();
}

// ==================== ErrorHandlerSpy ====================
// Replaces g_errorHandler with test spies. Restores original on destruction.
struct ErrorHandlerSpy {
    std::vector<std::string> fatalCalls;
    std::vector<std::string> errorCalls;
    std::vector<std::string> attentionCalls;
    std::vector<char> attentionResults;

    ErrorHandler original;
    bool restoreOnDestroy;

    explicit ErrorHandlerSpy(bool restore = true)
        : restoreOnDestroy(restore) {
        original = g_errorHandler;

        g_errorHandler.fatal = [this](const std::string& msg) {
            fatalCalls.push_back(msg);
        };
        g_errorHandler.error = [this](const std::string& msg) {
            errorCalls.push_back(msg);
        };
        g_errorHandler.attention = [this](const std::string& msg) -> char {
            attentionCalls.push_back(msg);
            char resp = 'y';
            attentionResults.push_back(resp);
            return resp;
        };
    }

    ~ErrorHandlerSpy() {
        if (restoreOnDestroy)
            g_errorHandler = original;
    }

    ErrorHandlerSpy(const ErrorHandlerSpy&) = delete;
    ErrorHandlerSpy& operator=(const ErrorHandlerSpy&) = delete;
};

} // namespace test_fixtures
