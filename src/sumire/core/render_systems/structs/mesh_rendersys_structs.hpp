#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct VertPushConstantData {
        glm::mat4 modelMatrix;
    };

    struct FragPushConstantData {
        uint32_t materialIdx;
    };
    
}