#pragma once

#include <sumire/core/rendering/sumi_transform3d.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <string>
#include <map>

namespace sumire {

    class SumiLight {
        public:
            using id_t = uint32_t;
            static id_t nextId;
            // SumiLight map is *ordered* so that it can be used to form a SSBO.
            using Map = std::map<id_t, SumiLight>;

            enum Type {
                // AMBIENT ? 
                PUNCTUAL_POINT = 0u,
                PUNCTUAL_SPOT = 1u, 
                PUNCTUAL_DIRECTIONAL = 2u,
            };

            static SumiLight createPointLight(glm::vec3 position);
            static SumiLight createSpotLight(float innerConeAngle, float outerConeAngle);
            static SumiLight createDirectionalLight(glm::vec3 rotation);

            SumiLight(const SumiLight &) = delete;
            SumiLight &operator=(const SumiLight &) = delete;
            SumiLight(SumiLight&&) = default;
            SumiLight &operator=(SumiLight&&) = default;

            // Shader Compatible Data 
            struct LightShaderData {
                // Shared - all light types
                glm::mat4 transform;
                glm::vec4 color;
                uint32_t type;
                // Type-specific 
                float range; // PUNCTUAL_POINT, PUNCTUAL_SPOT
                float lightAngleScale; // PUNCTUAL_SPOT
                float lightAngleOffset; // PUNCTUAL_SPOT
            };
            LightShaderData getShaderData();

            // Internal Data
            std::string name = "Unnamed Light";
            SumiLight::Type type;
            Transform3DComponent transform{};
            glm::vec4 color{ 1.0f };
            float innerConeAngle = 0.0f;
            float outerConeAngle = glm::pi<float>() / 4.0;
            float range = 0;

            const id_t getId() const { return id; }

        private:
            SumiLight(id_t lightId) : id{ lightId } {
                name = "Unnamed Light " + std::to_string(id);
            };

            void coneToLightAngle(
                float innerConeAngle, float outerConeAngle, 
                float &lightAngleScale, float &lightAngleOffset
            );

            id_t id;
        
    };
}