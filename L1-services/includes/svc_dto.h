/*
 *  tguide — svc_dto.h
 *  Data Transfer Objects for the service layer.
 *  Decouples L2 UI from L0-core data types.
 *  This file must NOT include any L0-core headers.
 *
 *  written by voidoxin
 */

#pragma once
#include <string>
#include <vector>

namespace SvcDTO {

    struct CategoryDTO {
        int         id;
        std::string name;
        std::string description;
    };

    struct ToolDTO {
        int         id;
        std::string name;
        std::string category;
        std::string short_desc;
        std::string description;
        std::string flags_all;
    };

    struct VulnerabilityDTO {
        int         id;
        std::string name;
        std::string metasploit;
        std::string discovered_date;
        std::string discoverer;
        std::string severity;
        std::string access;
        std::string platform;
        std::string service;
        std::string description;
        std::string danger;
    };

    struct ModuleDTO {
        int         id;
        std::string name;
        std::string path;
        std::string platform;
        std::string type;
        std::string description;
        std::string API;
        bool        mode;
        bool        loud;
        std::string output;

        ModuleDTO() : mode(false), loud(false) {}
    };

    struct ToolFlagDTO {
        int         id;
        int         tool_id;
        std::string name;
        std::string description;
        std::string loud;
        bool        root;
        std::string protocols;
    };

    struct TemplateDTO {
        int         id;
        int         tool_id;
        std::string template_name;
        std::string description;
        bool        root;
        std::string protocols;
        bool        flag;
    };

    struct OptionDTO {
        int         id;
        int         vuln_id;
        std::string name;
        std::string value;
    };

    struct SavedCommandDTO {
        int         id;
        int         tool_id;
        std::string command;
        std::string note;
        std::string tool_name;       // resolved by L1 service from tool_id
    };

} // namespace SvcDTO
