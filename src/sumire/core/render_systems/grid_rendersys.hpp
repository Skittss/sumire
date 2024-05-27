#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/rendering/general/sumi_camera.hpp>
#include <sumire/core/rendering/general/sumi_frame_info.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sumire {

    class GridRendersys {
        public:

            struct GridUBOdata {
                float opacity{1.0f};
                float tileSize{10.0f};
                float fogNear{0.01f};
                float fogFar{1.0f};
                alignas(16) glm::vec3 minorLineCol{0.2f};
                alignas(16) glm::vec3 xCol{1.0f, 0.2f, 0.2f};
                alignas(16) glm::vec3 zCol{0.2f, 0.2f, 1.0f};
            };
        
            struct GridMinimalVertex {
                glm::vec3 pos{};
                glm::vec2 uv{};

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            GridRendersys(
                SumiDevice& device, 
                VkRenderPass renderPass,
                uint32_t subpassIdx,
                VkDescriptorSetLayout globalDescriptorSetLayout
            );
            ~GridRendersys();

            GridRendersys(const GridRendersys&) = delete;
            GridRendersys& operator=(const GridRendersys&) = delete;

            void render(
                VkCommandBuffer commandBuffer, 
                FrameInfo &frameInfo, 
                GridRendersys::GridUBOdata &uniforms
            );

        private:
            void createGridQuadBuffers();
            void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
            void createPipeline(VkRenderPass renderPass, uint32_t subpassIdx);

            void bindGridQuadBuffers(VkCommandBuffer &commandBuffer);

            SumiDevice& sumiDevice;

            std::unique_ptr<SumiBuffer> quadVertexBuffer;
            std::unique_ptr<SumiBuffer> quadIndexBuffer;
            
            std::unique_ptr<SumiDescriptorPool> gridDescriptorPool;
            std::vector<VkDescriptorSet> gridDescriptorSets;
            std::vector<std::unique_ptr<SumiBuffer>> gridUniformBuffers;

            std::unique_ptr<SumiPipeline> sumiPipeline;
            VkPipelineLayout pipelineLayout;
        };
}