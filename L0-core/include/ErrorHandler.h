#pragma once
#include <string>
#include <functional>

struct ErrorHandler {
    std::function<void(const std::string&)> fatal;
    std::function<void(const std::string&)> error;
    std::function<char(const std::string&)> attention;
};

extern ErrorHandler g_errorHandler;
