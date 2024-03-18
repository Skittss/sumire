#pragma once

#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/sumi_transform3d.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace sumire {

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