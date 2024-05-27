#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/rendering/general/sumi_camera.hpp>
#include <sumire/core/rendering/general/sumi_frame_info.hpp>

#include <memory>
#include <vector>

namespace sumire {

    // A Forward-rendering system for rendering mesh data
    class MeshRenderSys {
        public:
            MeshRenderSys(
                SumiDevice& device,
                VkRenderPass renderPass,
                uint32_t subpassIdx,
                VkDescriptorSetLayout globalDescriptorSetLayout
            );
            ~MeshRenderSys();

            MeshRenderSys(const MeshRenderSys&) = delete;
            MeshRenderSys& operator=(const MeshRenderSys&) = delete;

            void renderObjects(VkCommandBuffer commandBuffer, FrameInfo &frameInfo);

        private:
            void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
            void createPipelines(VkRenderPass renderPass, uint32_t subpassIdx);

            SumiDevice& sumiDevice;

            // Keep model descriptor layout handles alive for the lifespan of this rendersystem
            std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout;
            std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout;
            std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout;

            // Pipelines used all share the same layout, but are configured differently.
            VkPipelineLayout pipelineLayout;
            std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> pipelines;
        };
}