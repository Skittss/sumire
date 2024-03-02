#pragma once

#include <glm/glm.hpp>

// Note: An extra namespace level here may alleviate potential struct mix-ups if there are multiple
//       with the same name in different hpp files.
namespace sumire::structs {

    struct VertPushConstantData {
        glm::mat4 modelMatrix;
    };

    struct FragPushConstantData {
        uint32_t materialIdx;
    };
    
}