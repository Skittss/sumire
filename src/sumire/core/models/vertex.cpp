#include <sumire/core/models/vertex.hpp>

namespace sumire {
    std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0, 0 , VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, joint)});
        attributeDescriptions.push_back({1, 0 , VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight)});
        attributeDescriptions.push_back({2, 0 , VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)});
        attributeDescriptions.push_back({3, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({4, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({5, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({6, 0 , VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv0)});
        attributeDescriptions.push_back({7, 0 , VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv1)});

        return attributeDescriptions;
    }
}