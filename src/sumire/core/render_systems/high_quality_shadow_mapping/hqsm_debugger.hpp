#pragma once

#include <sumire/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/hqsm_debugger_view.hpp>

namespace sumire {

    class HQSMdebugger {
    public:
        HQSMdebugger(
            SumiDevice& device,
            HighQualityShadowMapper* shadowMapper,
            SumiHZB* hzb,
            VkRenderPass renderPass
        );
        ~HQSMdebugger();

        void renderDebugView(VkCommandBuffer commandBuffer, HQSMdebuggerView debuggerView);
        void renderHzbDebugInfo(VkCommandBuffer commandBuffer);
        void renderLightCountDebugInfo(VkCommandBuffer commandBuffer);
        void updateScreenBounds(SumiHZB* hzb);

        // ---- View Configs
        HQSMlightCountListSource lightCountDebugListSource = 
            HQSMlightCountListSource::HQSM_LIGHT_COUNT_LIGHT_MASK;

    private:
        void initDescriptorLayouts();
        void initDescriptors(SumiHZB* hzb);
        void updateDescriptors(SumiHZB* hzb);
        void createPipelineLayouts();
        void createPipelines(VkRenderPass renderPass);

        SumiDevice& sumiDevice;
        HighQualityShadowMapper* shadowMapper;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;

        // ---- HZB View
        void initHzbDebugDescriptorSet(SumiHZB* hzb);
        void updateHzbDebugDescriptorSet(SumiHZB* hzb);
        void createHzbDebugPipelineLayout();
        void createHzbDebugPipeline(VkRenderPass renderPass);

        std::unique_ptr<SumiPipeline> hzbDebugPipeline;
        VkPipelineLayout hzbDebugPipelineLayout = VK_NULL_HANDLE;

        std::unique_ptr<SumiDescriptorSetLayout> hzbDebugDescriptorSetLayout;
        VkDescriptorSet hzbDebugDescriptorSet = VK_NULL_HANDLE;

        // ---- Light Count View
        void initLightCountDebugDescriptorSet();
        void updateLightCountDebugDescriptorSet();
        void createLightCountDebugPipelineLayout();
        void createLightCountDebugPipeline(VkRenderPass renderPass);

        std::unique_ptr<SumiPipeline> lightCountDebugPipeline;
        VkPipelineLayout lightCountDebugPipelineLayout = VK_NULL_HANDLE;

        std::unique_ptr<SumiDescriptorSetLayout> lightCountDebugDescriptorSetLayout;
        VkDescriptorSet lightCountDebugDescriptorSet = VK_NULL_HANDLE;

    };

}