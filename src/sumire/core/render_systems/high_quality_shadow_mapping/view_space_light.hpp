#pragma once

#include <sumire/core/rendering/sumi_light.hpp>

#include <glm/glm.hpp>

namespace sumire::structs {

    struct viewSpaceLight {
        SumiLight* lightPtr = nullptr;
        glm::vec3 viewSpacePosition{ 0.0f };
        float viewSpaceDepth = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 0.0f;
    };

}