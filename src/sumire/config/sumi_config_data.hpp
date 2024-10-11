#pragma once

#include <sumire/config/settings/graphics_settings.hpp>
#include <sumire/config/settings/keybind_settings.hpp>
#include <sumire/config/settings/profiling_settings.hpp>
#include <sumire/config/settings/shader_settings.hpp>

namespace sumire {

    //struct ResolutionData {
    //    uint32_t WIDTH = 1920u;
    //    uint32_t HEIGHT = 1080u;
    //};

    //struct PersistentGraphicsDeviceData {
    //    uint32_t IDX = 0;
    //    std::string NAME = "unset";
    //};

    //struct SumiConfigData {
    //    PersistentGraphicsDeviceData GRAPHICS_DEVICE{};
    //    ResolutionData RESOLUTION{};
    //    bool CPU_PROFILING = false;
    //    bool GPU_PROFILING = false;
    //    bool DEBUG_SHADERS = false;
    //    bool SHADER_HOT_RELOADING = false;
    //    bool VSYNC = false;
    //    uint32_t MAX_N_LIGHTS = 1024u;
    //};

    struct SumiConfigData {
        GraphicsSettings  graphics  {};
        KeybindSettings   keybinds  {};
        ProfilingSettings profiling {};
        ShaderSettings    shaders   {};
    };

}