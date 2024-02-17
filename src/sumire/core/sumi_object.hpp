#pragma once

#include <sumire/core/sumi_model.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace sumire {

    struct Transform3DComponent {
        glm::vec3 translation{};
        float scale{1.0f};
        glm::vec3 rotation{};

        glm::mat4 mat4() {
            auto transform = glm::translate(glm::mat4{1.0f}, translation);

            // TODO: Euler and Tait-bryan angles, Quaternions and Axis angles.
            // TODO: This is also very slow. Replace with hardcoded combined matrix for Y X Z, etc.
            transform = glm::rotate(transform, rotation.y, {0.0, 1.0, 0.0});
            transform = glm::rotate(transform, rotation.x, {1.0, 0.0, 0.0});
            transform = glm::rotate(transform, rotation.z, {0.0, 0.0, 1.0});

            transform = glm::scale(transform, glm::vec3(scale));
            return transform;
        }
    };

    class SumiObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, SumiObject>;

        static SumiObject createObject() {
            static id_t currentId = 0;
            return SumiObject{currentId++};
        }

        SumiObject(const SumiObject &) = delete;
        SumiObject &operator=(const SumiObject &) = delete;
        SumiObject(SumiObject&&) = default;
        SumiObject &operator=(SumiObject&&) = default;

        const id_t getId() { return id; }

        std::shared_ptr<SumiModel> model{};
        glm::vec3 colour{};
        Transform3DComponent transform{};

    private:
        SumiObject(id_t objId) : id{objId} {}

        id_t id;
    };
}