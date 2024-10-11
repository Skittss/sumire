#pragma once 

#include <cstdint>
#include <string>

namespace sumire {

    // ---- User-Accessible Graphics Settings ------------------------------------------------------
    struct ResolutionData {
        uint32_t WIDTH  = 1920u;
        uint32_t HEIGHT = 1080u;
    };

    struct PersistentGraphicsDeviceData {
        uint32_t IDX     = 0;
        std::string NAME = "unset";
    };

    struct UserGraphicsSettings {
        PersistentGraphicsDeviceData GRAPHICS_DEVICE{};
        ResolutionData RESOLUTION{};
        bool VSYNC = false;
    };

    // ---- Engine Graphics Settings ---------------------------------------------------------------

    struct InternalGraphicsSettings {
        uint32_t MAX_N_LIGHTS = 1024u;
    };

    // ---- All ------------------------------------------------------------------------------------
    struct GraphicsSettings {
        UserGraphicsSettings user{};
        InternalGraphicsSettings internal{};
    };

}