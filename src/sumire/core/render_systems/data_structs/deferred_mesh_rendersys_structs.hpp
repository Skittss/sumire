#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct VertPushConstantData {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
    };

    struct FragPushConstantData {
        uint32_t materialIdx;
    };

    struct CompositePushConstantData {
        uint32_t nLights;
    };
    
}