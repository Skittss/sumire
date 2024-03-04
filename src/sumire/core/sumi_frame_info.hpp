#pragma once

#include <sumire/core/sumi_camera.hpp>
#include <sumire/core/sumi_object.hpp>

#include <vulkan/vulkan.h>

namespace sumire {

    struct FrameInfo {
        int frameIdx;
        float frameTime;
        float cumulativeFrameTime;
        VkCommandBuffer commandBuffer;
        SumiCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        SumiObject::Map &objects;
    };

}