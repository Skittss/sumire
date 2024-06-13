#pragma once

#include <sumire/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.hpp>

namespace sumire {

    class HQSMdebugger {
    public:
        HQSMdebugger(
            SumiDevice& device,
            HighQualityShadowMapper* shadowMapper,
            VkRenderPass renderPass
        );
        ~HQSMdebugger() = default;

        void renderLightCountDebugInfo(VkCommandBuffer commandBuffer);

    private:
        void initDescriptorLayouts();
        void initLightCountDebugDescriptorSet();
        void updateDescriptors();
        void updateLightCountDebugDescriptorSet();
        void createPipelineLayouts();
        void createLightCountDebugPipelineLayout();
        void createPipelines(VkRenderPass renderPass);
        void createLightCountDebugPipeline(VkRenderPass renderPass);

        SumiDevice& sumiDevice;
        HighQualityShadowMapper* shadowMapper;

        std::unique_ptr<SumiPipeline> lightCountDebugPipeline;
        VkPipelineLayout lightCountDebugPipelineLayout = VK_NULL_HANDLE;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;

        std::unique_ptr<SumiDescriptorSetLayout> lightCountDebugDescriptorSetLayout;
        VkDescriptorSet lightCountDebugDescriptorSet = VK_NULL_HANDLE;

    };

}