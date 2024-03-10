#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <vulkan/vulkan.h>

namespace sumire {
    struct Vertex {
        glm::vec4 joint{};
        glm::vec4 weight{};
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec3 tangent{};
        glm::vec2 uv{};

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
                uv == other.uv
            );
        }
    };
}
