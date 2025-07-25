#pragma once

#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <rapidjson/document.h>

#define KBF_FIELD_PARSE_WARNING(field_name)										    \
        "Failed to parse value of json field \""	\
        field_name "\" - using default value."	

#define KBF_OBJECT_PARSE_WARNING(obj_name)										    \
        "Failed to parse value of json object \""	\
        obj_name "\" - using default value."	

#define KBF_FIELD_PARSE_WARNING_STR(field_name)										    \
        "Failed to parse value of json field \""	\
        + field_name + "\" - using default value."	

#define KBF_OBJECT_PARSE_WARNING_STR(obj_name)										    \
        "Failed to parse value of json object \""	\
        + obj_name + "\" - using default value."	

namespace kbf {

    bool parseObject(
        const rapidjson::Value& config,
        const std::string& memberName,
        const std::string& errorName
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsObject()) {
            return true;
        }
        else {
			DEBUG_STACK.push(KBF_OBJECT_PARSE_WARNING_STR(errorName), DebugStack::Color::WARNING);
            return false;
        }
    }

    bool parseBool(
        const rapidjson::Value& config,
        const std::string& memberName, 
        const std::string& errorName,
        bool* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsBool()) {
            *out = config[cstrName].GetBool();
            return true;
        }
        else {
			DEBUG_STACK.push(KBF_FIELD_PARSE_WARNING_STR(errorName), DebugStack::Color::WARNING);
            return false;
        }
    }

    bool parseString(
        const rapidjson::Value& config,
        const std::string& memberName,
        const std::string& errorName,
        std::string* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsString()) {
            *out = config[cstrName].GetString();
            return true;
        }
        else {
			DEBUG_STACK.push(KBF_FIELD_PARSE_WARNING_STR(errorName), DebugStack::Color::WARNING);
            return false;
        }
    }

    bool parseUint(
        const rapidjson::Value& config, 
        const std::string& memberName,
        const std::string& errorName,
        uint32_t* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsUint()) {
            *out = config[cstrName].GetUint();
            return true;
        }
        else {
			DEBUG_STACK.push(KBF_FIELD_PARSE_WARNING_STR(errorName), DebugStack::Color::WARNING);
            return false;
        }
    }

    bool parseInt(
        const rapidjson::Value& config,
        const std::string& memberName,
        const std::string& errorName,
        int* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsInt()) {
            *out = config[cstrName].GetInt();
            return true;
        }
        else {
			DEBUG_STACK.push(KBF_FIELD_PARSE_WARNING_STR(errorName), DebugStack::Color::WARNING);
            return false;
        }
    }
}