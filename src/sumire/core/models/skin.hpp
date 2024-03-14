#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace sumire {

    struct Node;

    struct Skin {
        std::string name;
        Node *skeletonRoot{nullptr};
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Node*> joints;

        ~Skin() {
            skeletonRoot = nullptr;
            joints.clear();
        }
    };
    
}