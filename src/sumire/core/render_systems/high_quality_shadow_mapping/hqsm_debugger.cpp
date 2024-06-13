#include <sumire/core/render_systems/high_quality_shadow_mapping/hqsm_debugger.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/push_constant_structs_debug.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {

    HQSMdebugger::HQSMdebugger(
        SumiDevice& device,
        HighQualityShadowMapper* shadowMapper,
        VkRenderPass renderPass
    ) : sumiDevice{ device },
        shadowMapper{ shadowMapper } {
        createPipelineLayouts();
        createPipelines(renderPass);
    }

    void HQSMdebugger::renderLightCountDebugInfo(VkCommandBuffer commandBuffer) {
        lightCountDebugPipeline->bind(commandBuffer);

        structs::LightCountDebugPush push{};
        
        vkCmdPushConstants(
            commandBuffer,
            lightCountDebugPipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(structs::LightCountDebugPush),
            &push
        );

        std::array<VkDescriptorSet, 1> lightCountDebugDescriptors{
            lightCountDebugDescriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            lightCountDebugPipelineLayout,
            0, static_cast<uint32_t>(lightCountDebugDescriptors.size()),
            lightCountDebugDescriptors.data(),
            0, nullptr
        );

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void HQSMdebugger::initDescriptorLayouts() {
        descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2)
            .build();

        lightCountDebugDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Light count early buffer
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Light count final buffer
            .build();
    }

    void HQSMdebugger::initLightCountDebugDescriptorSet() {
        VkDescriptorBufferInfo earlyLightCountBufferInfo = shadowMapper->getLightCountEarlyBuffer()->descriptorInfo();
        VkDescriptorBufferInfo finalLightCountBufferInfo = shadowMapper->getLightCountFinalBuffer()->descriptorInfo();

        SumiDescriptorWriter(*lightCountDebugDescriptorSetLayout, *descriptorPool)
            .writeBuffer(0, &earlyLightCountBufferInfo)
            .writeBuffer(1, &finalLightCountBufferInfo)
            .build(lightCountDebugDescriptorSet);
    }

    void HQSMdebugger::updateDescriptors() {
        updateLightCountDebugDescriptorSet();
    }

    void HQSMdebugger::updateLightCountDebugDescriptorSet() {
        VkDescriptorBufferInfo earlyLightCountBufferInfo = shadowMapper->getLightCountEarlyBuffer()->descriptorInfo();
        VkDescriptorBufferInfo finalLightCountBufferInfo = shadowMapper->getLightCountFinalBuffer()->descriptorInfo();

        SumiDescriptorWriter(*lightCountDebugDescriptorSetLayout, *descriptorPool)
            .writeBuffer(0, &earlyLightCountBufferInfo)
            .writeBuffer(1, &finalLightCountBufferInfo)
            .overwrite(lightCountDebugDescriptorSet);
    }

    void HQSMdebugger::createPipelineLayouts() {
        createLightCountDebugPipelineLayout();
    }

    void HQSMdebugger::createLightCountDebugPipelineLayout() {
        VkPushConstantRange lightCountPipelinePushRange{};
        lightCountPipelinePushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        lightCountPipelinePushRange.offset     = 0;
        lightCountPipelinePushRange.size       = sizeof(structs::LightCountDebugPush);

        std::vector<VkPushConstantRange> lighCountPushConstantRanges{
            lightCountPipelinePushRange
        };

        std::vector<VkDescriptorSetLayout> lightCountDescriptorSetLayouts{
            lightCountDebugDescriptorSetLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo lightCullingPipelineLayoutInfo{};
        lightCullingPipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightCullingPipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(lightCountDescriptorSetLayouts.size());
        lightCullingPipelineLayoutInfo.pSetLayouts            = lightCountDescriptorSetLayouts.data();
        lightCullingPipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(lighCountPushConstantRanges.size());
        lightCullingPipelineLayoutInfo.pPushConstantRanges    = lighCountPushConstantRanges.data();

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &lightCullingPipelineLayoutInfo, nullptr, &lightCountDebugPipelineLayout),
            "[Sumire::HSQMdebugger] Failed to create light culling debug pipeline layout."
        );
    }

    void HQSMdebugger::createPipelines(VkRenderPass renderPass) {
        createLightCountDebugPipeline(renderPass);
    }

    void HQSMdebugger::createLightCountDebugPipeline(VkRenderPass renderPass) {

        PipelineConfigInfo lightCullingDebugPipelineInfo{};
        SumiPipeline::defaultPipelineConfigInfo(lightCullingDebugPipelineInfo);
        lightCullingDebugPipelineInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        lightCullingDebugPipelineInfo.attributeDescriptions.clear();
        lightCullingDebugPipelineInfo.bindingDescriptions.clear();
        lightCullingDebugPipelineInfo.pipelineLayout = lightCountDebugPipelineLayout;
        lightCullingDebugPipelineInfo.renderPass = renderPass;

        lightCountDebugPipeline = std::make_unique<SumiPipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_light_culling.vert"),
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_light_culling.frag"),
            lightCullingDebugPipelineInfo
        );

    }

}