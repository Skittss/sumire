#pragma once

#include <glm/gtc/matrix_transform.hpp>

struct Transform3DComponent {
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::vec3 rotation{};

    glm::mat4 mat4() {
        auto transform = glm::translate(glm::mat4{1.0f}, translation);

        // TODO: Euler and Tait-bryan angles, Quaternions and Axis angles.
        // TODO: This is also very slow. Replace with hardcoded combined matrix for Y X Z, etc.
        transform = glm::rotate(transform, rotation.y, {0.0, 1.0, 0.0});
        transform = glm::rotate(transform, rotation.x, {1.0, 0.0, 0.0});
        transform = glm::rotate(transform, rotation.z, {0.0, 0.0, 1.0});

        transform = glm::scale(transform, scale);
        return transform;
    }
};