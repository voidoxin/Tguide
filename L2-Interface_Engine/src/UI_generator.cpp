/*
 *  tguide — UI_generator.cpp
 *  written by voidoxin
 *
 *  Script Generator screen — interactive multi-step script builder.
 *  Users can add template commands from tools, insert custom bash lines,
 *  reorder steps, preview the merged script, and save as .sh file.
 */

#include "../includes/UI_generator.h"
#include "../includes/UI_tools.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../includes/UI_paginator.h"
#include "../../L1-services/includes/svc_generator.h"
#include "../../L1-services/includes/svc_tools.h"
#include "../../L1-services/includes/svc_savedScripts.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/config_manager.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>
#include <vector>

using namespace std;

namespace {

// helper: count lines in a string (for preview truncation)
static int countLines(const string& s) {
    int n = 0;
    for (char c : s) if (c == '\n') n++;
    return n + (s.empty() || s.back() != '\n' ? 1 : 0);
}

// ── display the current sequence ────────────────────────────────────────
static void showSequence(const vector<SvcGenerator::ScriptStep>& steps) {
    if (steps.empty()) {
        cout << "  (sequence is empty \u2014 add steps to build a script)\n";
        return;
    }

    cout << (colorsEnabled() ? Color::BOLD : "") << "  Current Sequence:\n"
         << (colorsEnabled() ? Color::RESET : "");

    for (size_t i = 0; i < steps.size(); i++) {
        const auto& s = steps[i];
        cout << "  [" << (i + 1) << "] ";
        if (s.type == SvcGenerator::StepType::TEMPLATE_COMMAND) {
            cout << (colorsEnabled() ? Color::CYAN : "")
                 << s.toolName << " / " << s.templateName
                 << (colorsEnabled() ? Color::DIM : "")
                 << "  target=" << s.target;
            if (!s.port.empty()) cout << " port=" << s.port;
            cout << (colorsEnabled() ? Color::RESET : "");
        } else {
            cout << (colorsEnabled() ? Color::DIM : "")
                 << "bash: " << s.bashLine
                 << (colorsEnabled() ? Color::RESET : "");
        }
        cout << "\n";
    }
    cout << "\n";
}

// ── add a template command step ─────────────────────────────────────────
// Uses the existing tool browsing pattern: categories → tools → template → fill
static bool addTemplateStep(vector<SvcGenerator::ScriptStep>& steps) {
    // 1. Get categories
    auto cats = SvcTools::getCategoryList();
    if (cats.empty()) {
        cout << "\n  no categories available.\n\n";
        waitForEnter();
        return false;
    }

    // 2. Show categories – simple numbered list
    cout << "\n" << (colorsEnabled() ? Color::BOLD : "")
         << "  Select a Category:\n" << (colorsEnabled() ? Color::RESET : "");
    for (size_t i = 0; i < cats.size(); i++)
        cout << "    [" << (i + 1) << "] " << cats[i].name
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2014  " << cats[i].description
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    cout << "    [0] cancel\n  \u2192 ";

    string inp = readInput("");
    if (inp.empty() || isQuit(inp)) return false;
    if (isBack(inp) || isMenu(inp)) return false;
    int catIdx = toNumber(inp);
    if (catIdx < 1 || catIdx > static_cast<int>(cats.size())) {
        cout << "  invalid choice.\n";
        waitForEnter();
        return false;
    }

    // 3. Get tools in this category
    auto tools = SvcTools::getToolsByCategory(cats[static_cast<size_t>(catIdx - 1)].name);
    if (tools.empty()) {
        cout << "\n  no tools in this category.\n\n";
        waitForEnter();
        return false;
    }

    cout << "\n" << (colorsEnabled() ? Color::BOLD : "")
         << "  Select a Tool:\n" << (colorsEnabled() ? Color::RESET : "");
    for (size_t i = 0; i < tools.size(); i++)
        cout << "    [" << (i + 1) << "] " << tools[i].name
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2014  " << tools[i].short_desc
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    cout << "    [0] cancel\n  \u2192 ";

    inp = readInput("");
    if (inp.empty() || isQuit(inp)) return false;
    if (isBack(inp) || isMenu(inp)) return false;
    int toolIdx = toNumber(inp);
    if (toolIdx < 1 || toolIdx > static_cast<int>(tools.size())) {
        cout << "  invalid choice.\n";
        waitForEnter();
        return false;
    }
    auto tool = tools[static_cast<size_t>(toolIdx - 1)];

    // 4. Get templates for this tool
    auto tmpls = SvcTools::getTemplatesByToolId(tool.id);
    if (tmpls.empty()) {
        cout << "\n  no templates for this tool.\n\n";
        waitForEnter();
        return false;
    }

    cout << "\n" << (colorsEnabled() ? Color::BOLD : "")
         << "  Select a Template:\n" << (colorsEnabled() ? Color::RESET : "");
    for (size_t i = 0; i < tmpls.size(); i++) {
        cout << "    [" << (i + 1) << "] " << tmpls[i].template_name
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2014  " << tmpls[i].description
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    }
    cout << "    [0] cancel\n  \u2192 ";

    inp = readInput("");
    if (inp.empty() || isQuit(inp)) return false;
    if (isBack(inp) || isMenu(inp)) return false;
    int tmplIdx = toNumber(inp);
    if (tmplIdx < 1 || tmplIdx > static_cast<int>(tmpls.size())) {
        cout << "  invalid choice.\n";
        waitForEnter();
        return false;
    }
    auto tmpl = tmpls[static_cast<size_t>(tmplIdx - 1)];

    // 5. Prompt for target and port (reusing the template fill pattern)
    cout << "\n  target (IP/hostname): ";
    string target = readInput("");
    if (target.empty() || isQuit(target)) return false;
    target = sanitizeInput(target);

    string port;
    if (!tmpl.protocols.empty()) {
        cout << "  port: ";
        port = readInput("");
        if (!port.empty()) port = sanitizeInput(port);
    }

    // 6. Build the display command and confirm
    string cmd = SvcTools::buildCommand(tool, tmpl, target, port);
    cout << "\n  " << (colorsEnabled() ? Color::DIM : "")
         << "Command: " << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::BOLD : "") << cmd
         << (colorsEnabled() ? Color::RESET : "") << "\n\n"
         << "  add this step? (y/n): " << flush;

    // y/n confirm with retry loop
    while (true) {
        inp = readInput("");
        if (isQuit(inp)) { handleQuit(); return false; }
        if (isMenu(inp)) throw MenuJump{};
        if (isBack(inp)) { cout << "  step cancelled.\n"; waitForEnter(); return false; }
        if (inp == "y" || inp == "Y") break;
        if (inp == "n" || inp == "N") { cout << "  step cancelled.\n"; waitForEnter(); return false; }
        cout << "  please enter y or n: " << flush;
    }

    // 7. Add the step
    SvcGenerator::ScriptStep step;
    step.type = SvcGenerator::StepType::TEMPLATE_COMMAND;
    step.toolId = tool.id;
    step.templateId = tmpl.id;
    step.target = target;
    step.port = port;
    step.toolName = tool.name;
    step.templateName = tmpl.template_name;
    steps.push_back(step);

    cout << "  step added.\n\n";
    return true;
}

// ── add a custom bash line ──────────────────────────────────────────────
static bool addBashStep(vector<SvcGenerator::ScriptStep>& steps) {
    cout << "\n  enter bash command (or leave empty to cancel):\n  \u2192 ";
    string line = readInput("");
    if (line.empty() || isQuit(line)) return false;

    SvcGenerator::ScriptStep step;
    step.type = SvcGenerator::StepType::BASH_LINE;
    step.bashLine = line;
    steps.push_back(step);

    cout << "  bash line added.\n\n";
    return true;
}

// ── reorder steps ───────────────────────────────────────────────────────
static void reorderSteps(vector<SvcGenerator::ScriptStep>& steps) {
    if (steps.size() < 2) {
        cout << "\n  need at least 2 steps to reorder.\n";
        waitForEnter();
        return;
    }

    while (true) {
        showSequence(steps);
        cout << "  reorder: enter two numbers separated by space (e.g. \"1 3\")\n"
             << "  or 0 to cancel.\n  \u2192 ";

        string inp = readInput("");
        if (inp.empty() || isQuit(inp)) return;
        if (isBack(inp)) return;

        // parse "a b"
        size_t space = inp.find(' ');
        if (space == string::npos) {
            cout << "  use format: a b\n";
            waitForEnter();
            continue;
        }
        int a = toNumber(inp.substr(0, space));
        int b = toNumber(inp.substr(space + 1));
        if (a < 1 || b < 1 || a > static_cast<int>(steps.size()) || b > static_cast<int>(steps.size())) {
            cout << "  invalid indices (1-" << steps.size() << ").\n";
            waitForEnter();
            continue;
        }
        if (a == b) {
            cout << "  must be different indices.\n";
            waitForEnter();
            continue;
        }

        size_t ai = static_cast<size_t>(a - 1);
        size_t bi = static_cast<size_t>(b - 1);
        swap(steps[ai], steps[bi]);

        cout << "  swapped " << a << " \u2194 " << b << ".\n";
        waitForEnter();
        break;
    }
}

// ── preview and save ────────────────────────────────────────────────────
static void previewAndSave(const vector<SvcGenerator::ScriptStep>& steps, ConfigManager& cfg) {
    if (steps.empty()) {
        cout << "\n  sequence is empty \u2014 nothing to save.\n";
        waitForEnter();
        return;
    }

    string script = SvcGenerator::buildScript(steps);

    // Show preview
    cout << "\n" << (colorsEnabled() ? Color::BOLD : "")
         << "  Script Preview:\n"
         << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::DIM : "")
         << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
         << (colorsEnabled() ? Color::RESET : "");

    // Show first 20 lines, with scroll indication if longer
    istringstream stream(script);
    string line;
    int lineCount = 0;
    while (getline(stream, line) && lineCount < 20) {
        cout << "  " << line << "\n";
        lineCount++;
    }
    if (lineCount == 20) {
        cout << (colorsEnabled() ? Color::DIM : "")
             << "  ... (" << (countLines(script) - 20) << " more lines)"
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    }

    cout << (colorsEnabled() ? Color::DIM : "")
         << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
         << (colorsEnabled() ? Color::RESET : "")
         << "\n  save this script? (y/n): " << flush;

    // y/n confirm with retry loop
    while (true) {
        string inp = readInput("");
        if (isQuit(inp)) { handleQuit(); return; }
        if (isMenu(inp)) throw MenuJump{};
        if (isBack(inp)) { cout << "  script not saved.\n"; waitForEnter(); return; }
        if (inp == "y" || inp == "Y") break;
        if (inp == "n" || inp == "N") { cout << "  script not saved.\n"; waitForEnter(); return; }
        cout << "  please enter y or n: " << flush;
    }

    // Ask for a note/description
    cout << "  note (one-line description, optional): ";
    string note = readInput("");
    if (isQuit(note)) return;

    // Determine default save directory
    string cfgPath = cfg.get<string>("export.script_path", "");
    fs::path saveDir = cfgPath.empty() ? PathResolver::scriptsDir() : fs::path(cfgPath);

    // Generate filename from timestamp
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    char nameBuf[64];
    strftime(nameBuf, sizeof(nameBuf), "script_%Y%m%d_%H%M%S", &local);
    string defaultName = string(nameBuf) + ".sh";
    fs::path defaultPath = saveDir / defaultName;

    // Prompt for save path
    cout << "\n  Save path [Enter=default]:\n"
         << "  default: " << defaultPath.string() << "\n"
         << "  \u2192 ";
    string pathInput = readInput("");
    if (isQuit(pathInput)) return;

    fs::path scriptPath;
    if (pathInput.empty()) {
        scriptPath = defaultPath;
    } else {
        scriptPath = fs::path(pathInput);
        // If user entered an existing directory, append the default filename
        error_code ec2;
        if (fs::is_directory(scriptPath, ec2) || !scriptPath.has_extension()) {
            scriptPath /= defaultName;
        }
    }

    // Create parent directories if needed
    error_code ec;
    fs::create_directories(scriptPath.parent_path(), ec);

    // Write file
    ofstream outFile(scriptPath.string());
    if (!outFile) {
        cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
             << "! failed to open script file for writing: " << scriptPath.string()
             << (colorsEnabled() ? Color::RESET : "") << "\n";
        waitForEnter();
        return;
    }
    outFile << script;
    if (!outFile) {
        cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
             << "! failed to write script content."
             << (colorsEnabled() ? Color::RESET : "") << "\n";
        fs::remove(scriptPath);
        waitForEnter();
        return;
    }

    // Make executable
    fs::permissions(scriptPath,
                    fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec,
                    fs::perm_options::add, ec);
    if (ec) {
        cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
             << "! warning: could not set executable permission."
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    }

    // Register in saved scripts via service layer
    int id = SvcSavedScripts::saveScript(
        string(nameBuf),           // name without .sh
        scriptPath.string(),       // absolute path
        note                       // optional note
    );

    if (id != -1) {
        cout << "\n  " << (colorsEnabled() ? Color::CYAN : "")
             << "\u2713 script saved: " << scriptPath.filename().string()
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    } else {
        cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
             << "! file written but failed to register in saved scripts."
             << (colorsEnabled() ? Color::RESET : "") << "\n";
    }

    waitForEnter();
}

} // anonymous namespace

// ==================== PUBLIC ENTRY POINT ====================

void UIGenerator::show(ConfigManager& cfg) {
    vector<SvcGenerator::ScriptStep> steps;

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("script generator");
        UI::printDivider();
        cout << "\n";

        showSequence(steps);

        UI::printDivider();
        cout << "\n"
             << "  [1] Add Template Command   [3] Reorder Steps\n"
             << "  [2] Add Bash Line          [4] Preview & Save\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  [0] Back"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        string input = readInput("  \u2192 (m=menu) ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;

        if (input == "1") {
            addTemplateStep(steps);
        } else if (input == "2") {
            addBashStep(steps);
        } else if (input == "3") {
            reorderSteps(steps);
        } else if (input == "4") {
            previewAndSave(steps, cfg);
        } else {
            cout << "  " << "invalid choice." << "\n";
            waitForEnter();
        }
    }
}
