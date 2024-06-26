#pragma once

#include <sumire/core/rendering/general/sumi_camera.hpp>
#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/rendering/lighting/sumi_light.hpp>

#include <vulkan/vulkan.h>

namespace sumire {

    struct FrameInfo {
        int frameIdx;
        float frameTime;
        float cumulativeFrameTime;
        SumiCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        SumiObject::Map &objects;
        SumiLight::Map &lights;
    };

}