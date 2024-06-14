#include <sumire/config/sumi_config.hpp>
#include <sumire/config/field_parsers.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

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

            parseUint(graphicsDevice, "idx", "graphics_device.idx", &data.GRAPHICS_DEVICE.IDX);
            parseString(graphicsDevice, "name", "graphics_device.name", &data.GRAPHICS_DEVICE.NAME);
        }
        else {
            std::cout << OBJECT_PARSE_WARNING("graphics_device") << std::endl;
        }

        // resolution
        if (config.HasMember("resolution") && config["resolution"].IsObject()) {
            const rapidjson::Value& resolution = config["resolution"];

            parseUint(resolution, "width", "resolution.width", &data.RESOLUTION.WIDTH);
            parseUint(resolution, "height", "resolution.height", &data.RESOLUTION.HEIGHT);
        }
        else {
            std::cout << OBJECT_PARSE_WARNING("resolution") << std::endl;
        }

        // cpu_profiling
        parseBool(config, "cpu_profiling", "cpu_profiling", &data.CPU_PROFILING);

        // gpu_profiling
        parseBool(config, "gpu_profiling", "gpu_profiling.idx", &data.GPU_PROFILING);

        // debug_shaders
        parseBool(config, "debug_shaders", "debug_shaders", &data.DEBUG_SHADERS);

        // shader_hot_reloading
        parseBool(config, "shader_hot_reloading", "shader_hot_reloading", &data.SHADER_HOT_RELOADING);

        // vsync
        parseBool(config, "vsync", "vsync", &data.VSYNC);

        // max_n_lights
        parseUint(config, "max_n_lights", "max_n_lights", &data.MAX_N_LIGHTS);

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

        writer.Key("debug_shaders");
        writer.Bool(data.DEBUG_SHADERS);

        writer.Key("shader_hot_reloading");
        writer.Bool(data.SHADER_HOT_RELOADING);

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