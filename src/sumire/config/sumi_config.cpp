#include <sumire/config/sumi_config.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

#define FIELD_PARSE_WARNING_STR(field_name)										\
        "[Sumire::SumiConfig] WARNING: Failed to parse value of config field \""	\
        field_name "\" - using default value."	

#define OBJECT_PARSE_WARNING_STR(obj_name)										\
        "[Sumire::SumiConfig] WARNING: Failed to parse value of config object \""	\
        obj_name "\" - using default value."	

namespace sumire {

    SumiConfig::SumiConfig() : startupData{ loadConfigData() } {
        runtimeData = startupData;
    }

    SumiConfig::~SumiConfig() {

    }

    void SumiConfig::readConfig() {
        runtimeData = loadConfigData();
    }

    void SumiConfig::writeConfig() const {
        writeConfigData(runtimeData);
    }

    SumiConfigData SumiConfig::loadConfigData() {
        const std::string config_pth_string{ CONFIG_PATH };

        bool configExists = checkConfigFileExists();
        if (!configExists) {
            SumiConfigData defaultData = createDefaultConfig();
            std::cout << "[Sumire::SumiConfig] No existing config found at" + config_pth_string +
                ". A default config was created." << std::endl;
            return defaultData;
        }

        std::string json = readConfigFile();

        rapidjson::Document config;
        config.Parse(json.c_str());

        if (!config.IsObject() || config.HasParseError()) {
            throw std::runtime_error("[Sumire::SumiConfig] Failed to parse config. Fix or delete the config at " + config_pth_string);
        }

        SumiConfigData data = SumiConfigData{};

        // graphics_device
        if (config.HasMember("graphics_device") && config["graphics_device"].IsObject()) {
            const rapidjson::Value& graphicsDevice = config["graphics_device"];

            // graphics_device.idx
            if (graphicsDevice.HasMember("idx") && graphicsDevice["idx"].IsUint()) {
                data.GRAPHICS_DEVICE.IDX = graphicsDevice["idx"].GetUint();
            }
            else {
                std::cout << FIELD_PARSE_WARNING_STR("graphics_device.idx") << std::endl;
            }

            // graphics_device.name
            if (graphicsDevice.HasMember("name") && graphicsDevice["name"].IsString()) {
                data.GRAPHICS_DEVICE.NAME = graphicsDevice["name"].GetString();
            }
            else {
                std::cout << FIELD_PARSE_WARNING_STR("graphics_device.name") << std::endl;
            }
        }
        else {
            std::cout << OBJECT_PARSE_WARNING_STR("graphics_device") << std::endl;
        }

        // resolution
        if (config.HasMember("resolution") && config["resolution"].IsObject()) {
            const rapidjson::Value& resolution = config["resolution"];

            // resolution.width
            if (resolution.HasMember("width") && resolution["width"].IsUint()) {
                data.RESOLUTION.WIDTH = resolution["width"].GetUint();
            }
            else {
                std::cout << FIELD_PARSE_WARNING_STR("resolution.width") << std::endl;
            }

            // resolution.height
            if (resolution.HasMember("height") && resolution["height"].IsUint()) {
                data.RESOLUTION.HEIGHT = resolution["height"].GetUint();
            }
            else {
                std::cout << FIELD_PARSE_WARNING_STR("resolution.height") << std::endl;
            }

        }
        else {
            std::cout << OBJECT_PARSE_WARNING_STR("resolution") << std::endl;
        }

        // cpu_profiling
        if (config.HasMember("cpu_profiling") && config["cpu_profiling"].IsBool()) {
            data.CPU_PROFILING = config["cpu_profiling"].GetBool();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR("cpu_profiling") << std::endl;
        }

        // gpu_profiling
        if (config.HasMember("gpu_profiling") && config["gpu_profiling"].IsBool()) {
            data.GPU_PROFILING = config["gpu_profiling"].GetBool();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR("gpu_profiling") << std::endl;
        }

        // vsync
        if (config.HasMember("vsync") && config["vsync"].IsBool()) {
            data.VSYNC = config["vsync"].GetBool();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR("vsync") << std::endl;
        }

        // max_n_lights
        if (config.HasMember("max_n_lights") && config["max_n_lights"].IsUint()) {
            data.MAX_N_LIGHTS = config["max_n_lights"].GetUint();
        }
        else {
            std::cout << FIELD_PARSE_WARNING_STR("max_n_lights") << std::endl;
        }

        // Update config to fix any errors
        writeConfigData(data);

        std::cout << "[Sumire::SumiConfig] Read config from " + config_pth_string << std::endl;

        return data;
    }

    void SumiConfig::writeConfigData(const SumiConfigData& data) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();

        writer.Key("graphics_device");
        writer.StartObject();
        writer.Key("idx");
        writer.Uint(data.GRAPHICS_DEVICE.IDX);
        writer.Key("name");
        writer.String(data.GRAPHICS_DEVICE.NAME.c_str());
        writer.EndObject();

        writer.Key("resolution");
        writer.StartObject();
        writer.Key("width");
        writer.Uint(data.RESOLUTION.WIDTH);
        writer.Key("height");
        writer.Uint(data.RESOLUTION.HEIGHT);
        writer.EndObject();

        writer.Key("cpu_profiling");
        writer.Bool(data.CPU_PROFILING);

        writer.Key("gpu_profiling");
        writer.Bool(data.GPU_PROFILING);

        writer.Key("vsync");
        writer.Bool(data.VSYNC);

        writer.Key("max_n_lights");
        writer.Uint(data.MAX_N_LIGHTS);

        writer.EndObject();

        writeConfigFile(s.GetString());
    }

    SumiConfigData SumiConfig::createDefaultConfig() {
        SumiConfigData data = SumiConfigData{};
        writeConfigData(data);

        return data;
    }

    bool SumiConfig::checkConfigFileExists() const {
        std::ifstream f(CONFIG_PATH);
        return f.good();
    }

    std::string SumiConfig::readConfigFile() const {
        std::ifstream file{ CONFIG_PATH, std::ios::ate };

        if (!file.is_open()) {
            const std::string error_pth_out{ CONFIG_PATH };
            throw std::runtime_error("[Sumire::SumiConfig] Could not open config file for read at " + error_pth_out);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::string buffer(fileSize, ' ');
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    void SumiConfig::writeConfigFile(const char* json) const {
        std::ofstream file{ CONFIG_PATH, std::ios::trunc };

        if (!file.is_open()) {
            const std::string error_pth_out{ CONFIG_PATH };
            throw std::runtime_error("[Sumire::SumiConfig] Could not open config file for write at " + error_pth_out);
        }

        file << json;

        file.close();
    }

}