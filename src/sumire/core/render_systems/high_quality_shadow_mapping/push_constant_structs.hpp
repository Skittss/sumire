#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct findLightsApproxPush {
        glm::uvec2   shadowTileResolution;
        glm::uvec2   tileGroupResolution;
        glm::uvec2   lightMaskResolution;
        glm::uint    numZbinSlices;
        glm::float32 cameraNear;
        glm::float32 cameraFar;
    };

    struct findLightsAccuratePush {
        glm::uvec2   screenResolution;
        glm::uvec2   shadowTileResolution;
        glm::uvec2   tileGroupResolution;
        glm::uvec2   lightMaskResolution;
        glm::uint    numZbinSlices;
    };

    struct genDeferredShadowsPush {
        glm::uvec2   screenResolution;
        glm::uvec2   shadowTileResolution;
        glm::uvec2   tileGroupResolution;
        glm::uvec2   lightMaskResolution;
        glm::uint    numZbinSlices;
    };

}