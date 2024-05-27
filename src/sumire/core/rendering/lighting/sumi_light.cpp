#include <sumire/core/rendering/lighting/sumi_light.hpp>

#include <stdexcept>

namespace sumire {

    SumiLight::id_t SumiLight::nextId = 0u;

    SumiLight SumiLight::createPointLight(glm::vec3 position) {
        SumiLight light{nextId++};
        light.type = SumiLight::Type::PUNCTUAL_POINT;
        light.transform.setTranslation(position);

        // may need std::move? Unsure if treated like an r-value as pre-defined.
        return light;
    }

    SumiLight SumiLight::createSpotLight(float innerConeAngle, float outerConeAngle) {
        SumiLight light{nextId++};
        light.type = SumiLight::Type::PUNCTUAL_SPOT;
        light.innerConeAngle = innerConeAngle;
        light.outerConeAngle = outerConeAngle;

        return light;
    }

    SumiLight SumiLight::createDirectionalLight(glm::vec3 rotation) {
        SumiLight light{nextId++};
        light.type = SumiLight::Type::PUNCTUAL_DIRECTIONAL;
        light.transform.setRotation(rotation);

        return light;
    }

    SumiLight::LightShaderData SumiLight::getShaderData() {
        float lightAngleScale = 0;
        float lightAngleOffset = 0;
        coneToLightAngle(innerConeAngle, outerConeAngle, lightAngleScale, lightAngleOffset);

        glm::vec3 direction = glm::mat3(transform.modelMatrix()) * glm::vec3(0.0, 0.0, 1.0);

        return LightShaderData{
            color,
            transform.getTranslation(),
            direction,
            static_cast<uint32_t>(type),
            range,
            lightAngleScale,
            lightAngleOffset
        };
    }

    void SumiLight::coneToLightAngle(
        float innerConeAngle, float outerConeAngle, 
        float &lightAngleScale, float &lightAngleOffset
    ) {
        float cosInner = glm::cos(innerConeAngle);
        float cosOuter = glm::cos(outerConeAngle);

        //https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
        lightAngleScale  = 1.0f / glm::max(0.001f, cosInner - cosOuter);
        lightAngleOffset = -cosOuter * lightAngleScale;
    }

}