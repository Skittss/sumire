#pragma once

#include <sumire/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/hqsm_debugger_view.hpp>

namespace sumire {

    class HQSMdebugger {
    public:
        HQSMdebugger(
            SumiDevice& device,
            HighQualityShadowMapper* shadowMapper,
            VkRenderPass renderPass
        );
        ~HQSMdebugger();

        void renderDebugView(VkCommandBuffer commandBuffer, HQSMdebuggerView debuggerView);
        void renderLightCountDebugInfo(VkCommandBuffer commandBuffer);
        void updateScreenBounds();

    private:
        void initDescriptorLayouts();
        void initDescriptors();
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