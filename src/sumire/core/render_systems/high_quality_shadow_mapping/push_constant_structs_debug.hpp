#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct LightCountDebugPush {
        glm::uvec2 screenResolution;
        glm::uvec2 shadowTileResolution;
        glm::uvec2 tileGroupResolution;
        glm::uvec2 lightMaskResolution;
        glm::uint  listSource; // 0 = early, 1 = final, 2 = difference.
    };

    struct HzbDebugPush {
        glm::uvec2   screenResolution;
        glm::float32 minColRange;
        glm::float32 maxColRange;
    };
}