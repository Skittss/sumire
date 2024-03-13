#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <vulkan/vulkan.h>

namespace sumire {
    struct Vertex {
        glm::vec4 joint{};
        glm::vec4 weight{};
        glm::vec4 tangent{};
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv0{};
        glm::vec2 uv1{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    
        bool operator==(const Vertex &other) const {
            return (
                joint == other.joint &&
                weight == other.weight &&
                position == other.position && 
                color == other.color &&
                normal == other.normal &&
                tangent == other.tangent &&
                uv0 == other.uv0 &&
                uv1 == other.uv1
            );
        }
    };
}
