#pragma once

#include <vulkan/vulkan.h>

namespace sumire {

    class ImplPipeline {
    public:
        virtual void bind(VkCommandBuffer commandBuffer) = 0;
        virtual void queuePipelineRecreation() = 0;
    };

}