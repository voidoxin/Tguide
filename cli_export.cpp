/*
 *  tguide — cli_export.cpp
 *  Export functions for CLI display commands.
 *
 *  Supports text, JSON, YAML, and CSV output formats.
 *
 *  written by voidoxin
 */

#include "cli_export.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "libs/json.hpp"

using json = nlohmann::json;

// ==============================================================
// Helper: RFC 4180 CSV field quoting
// ==============================================================
static std::string csvQuote(const std::string& field) {
    // If the field contains a comma, double-quote, or newline, quote it
    if (field.find(',')  != std::string::npos ||
        field.find('"')  != std::string::npos ||
        field.find('\n') != std::string::npos) {
        std::string escaped;
        escaped.reserve(field.size() + 2);
        escaped += '"';
        for (char c : field) {
            if (c == '"')
                escaped += '"'; // double the quote
            escaped += c;
        }
        escaped += '"';
        return escaped;
    }
    return field;
}

// ==============================================================
// Helper: YAML value quoting
// ==============================================================
static std::string yamlQuote(const std::string& val) {
    // Always quote strings for simplicity and safety.
    // Escape special characters that are invalid in YAML double-quoted strings:
    //   \0  → \\0   (null)
    //   \a  → \\a   (bell)
    //   \b  → \\b   (backspace)
    //   \t  → \\t   (tab)
    //   \n  → \\n   (newline)
    //   \r  → \\r   (carriage return)
    //   \"  → \\\"  (double quote)
    //   \\  → \\\\  (backslash)
    std::string result;
    result.reserve(val.size() + 2);
    result += '"';
    for (char c : val) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\0': result += "\\0";  break;
            case '\a': result += "\\a";  break;
            case '\b': result += "\\b";  break;
            case '\t': result += "\\t";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            default:   result += c;      break;
        }
    }
    result += '"';
    return result;
}

// ==============================================================
// exportScriptsToText
// ==============================================================
bool exportScriptsToText(const std::vector<SvcDTO::SavedScriptDTO>& scripts,
                         const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filePath << "\n";
        return false;
    }

    for (const auto& s : scripts) {
        out << "[" << s.id << "] " << s.name << "\n";
        if (!s.path.empty())
            out << "     Path: " << s.path << "\n";
        if (!s.note.empty())
            out << "     Note: " << s.note << "\n";
        out << "---\n";
    }
    out << scripts.size() << " script(s) total.\n";

    out.close();
    std::cout << "Exported to " << filePath << "\n";
    return true;
}

// ==============================================================
// exportScriptsToJson
// ==============================================================
bool exportScriptsToJson(const std::vector<SvcDTO::SavedScriptDTO>& scripts,
                         const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filePath << "\n";
        return false;
    }

    json j;
    json arr = json::array();
    for (const auto& s : scripts) {
        arr.push_back({
            {"id",   s.id},
            {"name", s.name},
            {"path", s.path},
            {"note", s.note}
        });
    }
    j["scripts"] = arr;

    out << j.dump(2) << "\n";
    out.close();
    std::cout << "Exported to " << filePath << "\n";
    return true;
}

// ==============================================================
// exportScriptsToYaml
// ==============================================================
bool exportScriptsToYaml(const std::vector<SvcDTO::SavedScriptDTO>& scripts,
                         const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filePath << "\n";
        return false;
    }

    out << "scripts:\n";
    for (const auto& s : scripts) {
        out << "  - id: "   << s.id   << "\n";
        out << "    name: " << yamlQuote(s.name) << "\n";
        out << "    path: " << yamlQuote(s.path) << "\n";
        out << "    note: " << yamlQuote(s.note) << "\n";
    }

    out.close();
    std::cout << "Exported to " << filePath << "\n";
    return true;
}

// ==============================================================
// exportScriptsToCsv
// ==============================================================
bool exportScriptsToCsv(const std::vector<SvcDTO::SavedScriptDTO>& scripts,
                         const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filePath << "\n";
        return false;
    }

    out << "id,name,path,note\n";
    for (const auto& s : scripts) {
        out << s.id << ","
            << csvQuote(s.name) << ","
            << csvQuote(s.path) << ","
            << csvQuote(s.note) << "\n";
    }

    out.close();
    std::cout << "Exported to " << filePath << "\n";
    return true;
}

// ==============================================================
// exportTextContent
// ==============================================================
bool exportTextContent(const std::string& content,
                        const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filePath << "\n";
        return false;
    }

    out << content;
    out.close();
    std::cout << "Exported to " << filePath << "\n";
    return true;
}
