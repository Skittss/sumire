#pragma once

#include <sumire/core/models/mesh.hpp>
#include <sumire/core/models/skin.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <memory>
#include <string>

namespace sumire {

    struct Node {
        uint32_t idx;
        Node *parent;
        std::vector<Node*> children;
        std::string name;
        std::unique_ptr<Mesh> mesh; // Optional

        // Node transform properties
        glm::mat4 matrix{ 1.0f };
        glm::vec3 translation{ 0.0f };
        glm::quat rotation;
        glm::vec3 scale{ 1.0f };

        glm::mat4 cachedLocalTransform{ 1.0f };
        glm::mat4 worldTransform{ 1.0f };
        glm::mat4 invWorldTransform{ 1.0f };
        glm::mat4 normalMatrix{ 1.0f };

        void setMatrix(glm::mat4 matrix);
        void setTranslation(glm::vec3 translation);
        void setRotation(glm::quat rotation);
        void setScale(glm::vec3 scale);

        glm::mat4 getLocalTransform();
        glm::mat4 getGlobalTransform();

        // Skinning (Optional)
        Skin *skin;
        int32_t skinIdx{-1};

        // Update matrices, skinning, and joints
        void applyTransformHierarchy();
        void updateRecursive();
        void update();
        bool needsUpdate = true;

        ~Node() {
            mesh = nullptr;
            parent = nullptr;
            skin = nullptr;
            children.clear();
        }
    };

}