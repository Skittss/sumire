#pragma once

#include "sumi_model.hpp"

#include <memory>

namespace sumire {

    class SumiObject {
    public:
        using id_t = unsigned int;

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
        glm::vec3 colour;
        glm::mat4 transform;

    private:
        SumiObject(id_t objId) : id{objId} {}

        id_t id;
    };
}