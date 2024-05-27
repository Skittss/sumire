#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/rendering/general/sumi_camera.hpp>
#include <sumire/core/rendering/general/sumi_frame_info.hpp>

#include <memory>
#include <vector>

namespace sumire {

    class PointLightRenderSys {
        public:
            PointLightRenderSys(
                SumiDevice& device, 
                VkRenderPass renderPass, 
                uint32_t subpassIdx, 
                VkDescriptorSetLayout globalDescriptorSetLayout
            );
            ~PointLightRenderSys();

            PointLightRenderSys(const PointLightRenderSys&) = delete;
            PointLightRenderSys& operator=(const PointLightRenderSys&) = delete;

            void render(VkCommandBuffer commandBuffer, FrameInfo &frameInfo);

        private:
            void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
            void createPipeline(VkRenderPass renderPass, uint32_t subpassIdx);

            SumiDevice& sumiDevice;

            std::unique_ptr<SumiPipeline> sumiPipeline;
            VkPipelineLayout pipelineLayout;
        };
}