#pragma once

#include <rapidjson/document.h>
#include <iostream>

#define FIELD_PARSE_WARNING(field_name)										    \
        "[Sumire::FieldParsers] WARNING: Failed to parse value of config field \""	\
        field_name "\" - using default value."	

#define OBJECT_PARSE_WARNING(obj_name)										    \
        "[Sumire::FieldParsers] WARNING: Failed to parse value of config object \""	\
        obj_name "\" - using default value."	

#define FIELD_PARSE_WARNING_STR(field_name)										    \
        "[Sumire::FieldParsers] WARNING: Failed to parse value of config field \""	\
        + field_name + "\" - using default value."	

#define OBJECT_PARSE_WARNING_STR(obj_name)										    \
        "[Sumire::FieldParsers] WARNING: Failed to parse value of config object \""	\
        + obj_name + "\" - using default value."	

namespace sumire {

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
            std::cout << OBJECT_PARSE_WARNING_STR(errorName) << std::endl;
            return false;
        }
    }

    void parseBool(
        const rapidjson::Value& config,
        const std::string& memberName, 
        const std::string& errorName,
        bool* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsBool()) {
            *out = config[cstrName].GetBool();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR(errorName) << std::endl;
        }
    }

    void parseString(
        const rapidjson::Value& config,
        const std::string& memberName,
        const std::string& errorName,
        std::string* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsString()) {
            *out = config[cstrName].GetString();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR(errorName) << std::endl;
        }
    }

    void parseUint(
        const rapidjson::Value& config, 
        const std::string& memberName,
        const std::string& errorName,
        uint32_t* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsUint()) {
            *out = config[cstrName].GetUint();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR(errorName) << std::endl;
        }
    }

    void parseInt(
        const rapidjson::Value& config,
        const std::string& memberName,
        const std::string& errorName,
        int* out
    ) {
        const char* cstrName = memberName.c_str();
        if (config.HasMember(cstrName) && config[cstrName].IsInt()) {
            *out = config[cstrName].GetInt();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR(errorName) << std::endl;
        }
    }
}