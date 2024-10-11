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

        std::string objNameStack = "";

        // GraphicsSettings ------------------------------------------------------------------------
        strStackAdd(objNameStack, "graphics");
        if (parseObject(config, "graphics", objNameStack)) {
            const rapidjson::Value& v_graphics = config["graphics"];

            // ::UserGraphicsSettings
            strStackAdd(objNameStack, "::user");
            if (parseObject(v_graphics, "user", objNameStack)) {
                const rapidjson::Value& v_userGraphicsSettings = v_graphics["user"];

                // ::PersistentGraphicsDeviceData
                strStackAdd(objNameStack, "::graphics_device");
                if (parseObject(v_userGraphicsSettings, "graphics_device", objNameStack)) {
                    const rapidjson::Value& v_graphicsDevice = v_userGraphicsSettings["graphics_device"];

                    auto& localConfigObj = data.graphics.user.GRAPHICS_DEVICE;
                    // .IDX
                    parseUint(v_graphicsDevice, "idx", objNameStack + ".idx", &localConfigObj.IDX);
                    // .IDX
                    parseString(v_graphicsDevice, "name", objNameStack + ".name", &localConfigObj.NAME);

                }
                strStackPop(objNameStack, "::graphics_device");

                // ::ResolutionData
                strStackAdd(objNameStack, "::resolution");
                if (parseObject(v_userGraphicsSettings, "resolution", objNameStack)) {
                    const rapidjson::Value& v_resolution = v_userGraphicsSettings["resolution"];

                    auto& localConfigObj = data.graphics.user.RESOLUTION;
                    // .WIDTH
                    parseUint(v_resolution, "width", objNameStack + ".width", &localConfigObj.WIDTH);
                    // .HEIGHT
                    parseUint(v_resolution, "height", objNameStack + ".height", &localConfigObj.HEIGHT);

                }
                strStackPop(objNameStack, "::resolution");

                // .VSYNC
                auto& localConfigObj = data.graphics.user;
                parseBool(v_userGraphicsSettings, "vsync", objNameStack + ".vsync", &localConfigObj.VSYNC);

            }
            strStackPop(objNameStack, "::user");

            // ::InternalGraphicsSettings
            strStackAdd(objNameStack, "::internal");
            if (parseObject(v_graphics, "internal", objNameStack)) {
                const rapidjson::Value& v_internalGraphicsSettings = v_graphics["internal"];

                // .MAX_N_LIGHTS
                auto& localConfigObj = data.graphics.internal;
                parseUint(v_internalGraphicsSettings, "max_n_lights", objNameStack + ".max_n_lights", &localConfigObj.MAX_N_LIGHTS);

            }
            strStackPop(objNameStack, "::internal");

        }
        strStackPop(objNameStack, "graphics");

        // KeybindSettings -------------------------------------------------------------------------
        strStackAdd(objNameStack, "keybinds");
        if (parseObject(config, "keybinds", objNameStack)) {
            const rapidjson::Value& v_keybinds = config["keybinds"];

            // ::DebugCameraKeybinds
            strStackAdd(objNameStack, "::debug_camera");
            if (parseObject(v_keybinds, "debug_camera", objNameStack)) {
                const rapidjson::Value& v_debugCameraKeybinds = v_keybinds["debug_camera"];

                auto& localConfigObj = data.keybinds.debugCamera;
                parseInt(v_debugCameraKeybinds, "move_left",     objNameStack + ".move_left",     &localConfigObj.MOVE_LEFT);
                parseInt(v_debugCameraKeybinds, "move_right",    objNameStack + ".move_right",    &localConfigObj.MOVE_RIGHT);
                parseInt(v_debugCameraKeybinds, "move_forward",  objNameStack + ".move_forward",  &localConfigObj.MOVE_FORWARD);
                parseInt(v_debugCameraKeybinds, "move_backward", objNameStack + ".move_backward", &localConfigObj.MOVE_BACKWARD);
                parseInt(v_debugCameraKeybinds, "move_up",       objNameStack + ".move_up",       &localConfigObj.MOVE_UP);
                parseInt(v_debugCameraKeybinds, "move_down",     objNameStack + ".move_down",     &localConfigObj.MOVE_DOWN);
                parseInt(v_debugCameraKeybinds, "look_left",     objNameStack + ".look_left",     &localConfigObj.LOOK_LEFT);
                parseInt(v_debugCameraKeybinds, "look_right",    objNameStack + ".look_right",    &localConfigObj.LOOK_RIGHT);
                parseInt(v_debugCameraKeybinds, "look_up",       objNameStack + ".look_up",       &localConfigObj.LOOK_UP);
                parseInt(v_debugCameraKeybinds, "look_down",     objNameStack + ".look_down",     &localConfigObj.LOOK_DOWN);
                parseInt(v_debugCameraKeybinds, "sprint",        objNameStack + ".sprint",        &localConfigObj.SPRINT);
                parseInt(v_debugCameraKeybinds, "toggle_cursor", objNameStack + ".toggle_cursor", &localConfigObj.TOGGLE_CURSOR);

            }
            strStackPop(objNameStack, "::debug_camera");

        }
        strStackPop(objNameStack, "keybinds");

        // Profiling -------------------------------------------------------------------------------
        strStackAdd(objNameStack, "profiling");
        if (parseObject(config, "profiling", objNameStack)) {
            const rapidjson::Value& v_profiling = config["profiling"];

            auto& localConfigObj = data.profiling;
            // .CPU_PROFILING
            parseBool(v_profiling, "cpu_profiling", objNameStack + ".cpu_profiling", &localConfigObj.CPU_PROFILING);
            // .GPU_PROFILING
            parseBool(v_profiling, "gpu_profiling", objNameStack + ".gpu_profiling", &localConfigObj.GPU_PROFILING);

        }
        strStackPop(objNameStack, "profiling");

        // Shaders ---------------------------------------------------------------------------------
        strStackAdd(objNameStack, "shaders");
        if (parseObject(config, "shaders", objNameStack)) {
            const rapidjson::Value& v_shaders = config["shaders"];

            auto& localConfigObj = data.shaders;
            // .DEBUG_SHADERS
            parseBool(v_shaders, "debug_shaders", objNameStack + ".debug_shaders", &localConfigObj.DEBUG_SHADERS);
            // .SHADER_HOT_RELOADING
            parseBool(v_shaders, "shader_hot_reloading", objNameStack + ".shader_hot_reloading", &localConfigObj.SHADER_HOT_RELOADING);

        }
        strStackPop(objNameStack, "shaders");

        // Update config to fix any errors
        writeConfigData(data);

        std::cout << "[Sumire::SumiConfig] Read config from " + config_pth_string << std::endl;

        return data;
    }

    void SumiConfig::writeConfigData(const SumiConfigData& data) const {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

        writer.StartObject();

        writer.Key("graphics");
        writer.StartObject();
            writer.Key("user");
            writer.StartObject();
                writer.Key("graphics_device");
                writer.StartObject();
                    writer.Key("idx");
                    writer.Uint(data.graphics.user.GRAPHICS_DEVICE.IDX);
                    writer.Key("name");
                    writer.String(data.graphics.user.GRAPHICS_DEVICE.NAME.c_str());
                writer.EndObject();
                writer.Key("resolution");
                writer.StartObject();
                    writer.Key("width");
                    writer.Uint(data.graphics.user.RESOLUTION.WIDTH);
                    writer.Key("height");
                    writer.Uint(data.graphics.user.RESOLUTION.HEIGHT);
                writer.EndObject();
                writer.Key("vsync");
                writer.Bool(data.graphics.user.VSYNC);
            writer.EndObject();
            writer.Key("internal");
            writer.StartObject();
                writer.Key("max_n_lights");
                writer.Uint(data.graphics.internal.MAX_N_LIGHTS);
            writer.EndObject();
        writer.EndObject();

        writer.Key("keybinds");
        writer.StartObject();
            writer.Key("debug_camera");
            writer.StartObject();
                writer.Key("move_left");
                writer.Int(data.keybinds.debugCamera.MOVE_LEFT);
                writer.Key("move_right");
                writer.Int(data.keybinds.debugCamera.MOVE_RIGHT);
                writer.Key("move_forward");
                writer.Int(data.keybinds.debugCamera.MOVE_FORWARD);
                writer.Key("move_backward");
                writer.Int(data.keybinds.debugCamera.MOVE_BACKWARD);
                writer.Key("move_up");
                writer.Int(data.keybinds.debugCamera.MOVE_UP);
                writer.Key("move_down");
                writer.Int(data.keybinds.debugCamera.MOVE_DOWN);
                writer.Key("look_left");
                writer.Int(data.keybinds.debugCamera.LOOK_LEFT);
                writer.Key("look_right");
                writer.Int(data.keybinds.debugCamera.LOOK_RIGHT);
                writer.Key("look_up");
                writer.Int(data.keybinds.debugCamera.LOOK_UP);
                writer.Key("look_down");
                writer.Int(data.keybinds.debugCamera.LOOK_DOWN);
                writer.Key("sprint");
                writer.Int(data.keybinds.debugCamera.SPRINT);
                writer.Key("toggle_cursor");
                writer.Int(data.keybinds.debugCamera.TOGGLE_CURSOR);
            writer.EndObject();
        writer.EndObject();

        writer.Key("profiling");
        writer.StartObject();
            writer.Key("cpu_profiling");
            writer.Bool(data.profiling.CPU_PROFILING);
            writer.Key("gpu_profiling");
            writer.Bool(data.profiling.GPU_PROFILING);
        writer.EndObject();

        writer.Key("shaders");
        writer.StartObject();
            writer.Key("debug_shaders");
            writer.Bool(data.shaders.DEBUG_SHADERS);
            writer.Key("shader_hot_reloading");
            writer.Bool(data.shaders.SHADER_HOT_RELOADING);
        writer.EndObject();

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