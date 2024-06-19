#include <sumire/core/render_systems/high_quality_shadow_mapping/hqsm_debugger.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/push_constant_structs_debug.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {

    HQSMdebugger::HQSMdebugger(
        SumiDevice& device,
        HighQualityShadowMapper* shadowMapper,
        SumiHZB* hzb,
        VkRenderPass renderPass
    ) : sumiDevice{ device },
        shadowMapper{ shadowMapper } {
        initDescriptorLayouts();
        initDescriptors(hzb);
        createPipelineLayouts();
        createPipelines(renderPass);
    }

    HQSMdebugger::~HQSMdebugger() {
        vkDestroyPipelineLayout(sumiDevice.device(), hzbDebugPipelineLayout, nullptr);
        vkDestroyPipelineLayout(sumiDevice.device(), lightCountDebugPipelineLayout, nullptr);
    }

    void HQSMdebugger::renderDebugView(VkCommandBuffer commandBuffer, HQSMdebuggerView debuggerView) {
        switch (debuggerView) {
        case HQSM_DEBUG_HZB:
            renderHzbDebugInfo(commandBuffer);
            break;
        case HQSMdebuggerView::HQSM_DEBUG_LIGHT_COUNT:
            renderLightCountDebugInfo(commandBuffer);
            break;
        case HQSMdebuggerView::HQSM_DEBUG_LIGHT_CULLING:
            break;
        case HQSMdebuggerView::HQSM_DEBUG_NONE:
        default:
            break;
        }
    }

    void HQSMdebugger::renderHzbDebugInfo(VkCommandBuffer commandBuffer) {
        hzbDebugPipeline->bind(commandBuffer);

        structs::HzbDebugPush push{};
        push.screenResolution = shadowMapper->getResolution();
        push.minColRange      = 0.90f;
        push.maxColRange      = 1.0f;

        vkCmdPushConstants(
            commandBuffer,
            hzbDebugPipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(structs::HzbDebugPush),
            &push
        );

        std::array<VkDescriptorSet, 1> hzbDebugDescriptors{
            hzbDebugDescriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            hzbDebugPipelineLayout,
            0, static_cast<uint32_t>(hzbDebugDescriptors.size()),
            hzbDebugDescriptors.data(),
            0, nullptr
        );

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void HQSMdebugger::renderLightCountDebugInfo(VkCommandBuffer commandBuffer) {
        lightCountDebugPipeline->bind(commandBuffer);

        structs::LightCountDebugPush push{};
        push.screenResolution     = shadowMapper->getResolution();;
        push.shadowTileResolution = shadowMapper->getShadowTileResolution();
        push.tileGroupResolution  = shadowMapper->getTileGroupResolution();
        push.lightMaskResolution  = shadowMapper->getLightMaskResolution();
        push.listSource           = static_cast<uint32_t>(lightCountDebugListSource);
        
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

    void HQSMdebugger::updateScreenBounds(SumiHZB* hzb) {
        updateDescriptors(hzb);
    }

    void HQSMdebugger::initDescriptorLayouts() {
        descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(
                1 + // HZB View
                4   // Light Count View
            )
            // ---- HZB View ----------------------------------------------
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            // ---- Light Count View --------------------------------------
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4)
            // ------------------------------------------------------------
            .build();

        hzbDebugDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        lightCountDebugDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Light Mask buffer
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Tile Group Light Mask buffer
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Light count early buffer
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Light count final buffer
            .build();
    }

    void HQSMdebugger::initDescriptors(SumiHZB* hzb) {
        initHzbDebugDescriptorSet(hzb);
        initLightCountDebugDescriptorSet();
    }

    void HQSMdebugger::updateDescriptors(SumiHZB* hzb) {
        updateHzbDebugDescriptorSet(hzb);
        updateLightCountDebugDescriptorSet();
    }

    void HQSMdebugger::createPipelineLayouts() {
        createHzbDebugPipelineLayout();
        createLightCountDebugPipelineLayout();
    }

    void HQSMdebugger::createPipelines(VkRenderPass renderPass) {
        createHzbDebugPipeline(renderPass);
        createLightCountDebugPipeline(renderPass);
    }

    // ---- HZB View ---------------------------------------------------------------------------------------------
    void HQSMdebugger::initHzbDebugDescriptorSet(SumiHZB* hzb) {
        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler = shadowMapper->getAttachmentSampler();
        hzbInfo.imageView = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*hzbDebugDescriptorSetLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .build(hzbDebugDescriptorSet);
    }

    void HQSMdebugger::updateHzbDebugDescriptorSet(SumiHZB* hzb) {
        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler = shadowMapper->getAttachmentSampler();
        hzbInfo.imageView = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*hzbDebugDescriptorSetLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .overwrite(hzbDebugDescriptorSet);
    }

    void HQSMdebugger::createHzbDebugPipelineLayout() {
        VkPushConstantRange hzbPipelinePushRange{};
        hzbPipelinePushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        hzbPipelinePushRange.offset = 0;
        hzbPipelinePushRange.size = sizeof(structs::HzbDebugPush);

        std::vector<VkPushConstantRange> hzbPushConstantRanges{
            hzbPipelinePushRange
        };

        std::vector<VkDescriptorSetLayout> hzbDescriptorSetLayouts{
            hzbDebugDescriptorSetLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo hzbPipelineLayoutInfo{};
        hzbPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        hzbPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(hzbDescriptorSetLayouts.size());
        hzbPipelineLayoutInfo.pSetLayouts = hzbDescriptorSetLayouts.data();
        hzbPipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(hzbPushConstantRanges.size());
        hzbPipelineLayoutInfo.pPushConstantRanges = hzbPushConstantRanges.data();

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &hzbPipelineLayoutInfo, nullptr, &hzbDebugPipelineLayout),
            "[Sumire::HSQMdebugger] Failed to create light culling debug pipeline layout."
        );
    }

    void HQSMdebugger::createHzbDebugPipeline(VkRenderPass renderPass) {

        PipelineConfigInfo hzbDebugPipelineInfo{};
        SumiPipeline::defaultPipelineConfigInfo(hzbDebugPipelineInfo);
        SumiPipeline::enableAlphaBlending(hzbDebugPipelineInfo);
        hzbDebugPipelineInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        hzbDebugPipelineInfo.attributeDescriptions.clear();
        hzbDebugPipelineInfo.bindingDescriptions.clear();
        hzbDebugPipelineInfo.pipelineLayout = hzbDebugPipelineLayout;
        hzbDebugPipelineInfo.renderPass = renderPass;

        hzbDebugPipeline = std::make_unique<SumiPipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_fs_triangle.vert"),
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_hzb.frag"),
            hzbDebugPipelineInfo
        );

    }

    // ---- Light Count View -------------------------------------------------------------------------------------
    void HQSMdebugger::initLightCountDebugDescriptorSet() {
        VkDescriptorBufferInfo lightMaskBufferInfo = shadowMapper->getLightMaskBuffer()->descriptorInfo();
        VkDescriptorBufferInfo tileGroupLightMaskBufferInfo = shadowMapper->getTileGroupLightMaskBuffer()->descriptorInfo();
        VkDescriptorBufferInfo earlyLightCountBufferInfo = shadowMapper->getLightCountEarlyBuffer()->descriptorInfo();
        VkDescriptorBufferInfo finalLightCountBufferInfo = shadowMapper->getLightCountFinalBuffer()->descriptorInfo();

        SumiDescriptorWriter(*lightCountDebugDescriptorSetLayout, *descriptorPool)
            .writeBuffer(0, &lightMaskBufferInfo)
            .writeBuffer(1, &tileGroupLightMaskBufferInfo)
            .writeBuffer(2, &earlyLightCountBufferInfo)
            .writeBuffer(3, &finalLightCountBufferInfo)
            .build(lightCountDebugDescriptorSet);
    }

    void HQSMdebugger::updateLightCountDebugDescriptorSet() {
        VkDescriptorBufferInfo lightMaskBufferInfo = shadowMapper->getLightMaskBuffer()->descriptorInfo();
        VkDescriptorBufferInfo tileGroupLightMaskBufferInfo = shadowMapper->getTileGroupLightMaskBuffer()->descriptorInfo();
        VkDescriptorBufferInfo earlyLightCountBufferInfo = shadowMapper->getLightCountEarlyBuffer()->descriptorInfo();
        VkDescriptorBufferInfo finalLightCountBufferInfo = shadowMapper->getLightCountFinalBuffer()->descriptorInfo();

        SumiDescriptorWriter(*lightCountDebugDescriptorSetLayout, *descriptorPool)
            .writeBuffer(0, &lightMaskBufferInfo)
            .writeBuffer(1, &tileGroupLightMaskBufferInfo)
            .writeBuffer(2, &earlyLightCountBufferInfo)
            .writeBuffer(3, &finalLightCountBufferInfo)
            .overwrite(lightCountDebugDescriptorSet);
    }

    void HQSMdebugger::createLightCountDebugPipelineLayout() {
        VkPushConstantRange lightCountPipelinePushRange{};
        lightCountPipelinePushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        lightCountPipelinePushRange.offset = 0;
        lightCountPipelinePushRange.size = sizeof(structs::LightCountDebugPush);

        std::vector<VkPushConstantRange> lighCountPushConstantRanges{
            lightCountPipelinePushRange
        };

        std::vector<VkDescriptorSetLayout> lightCountDescriptorSetLayouts{
            lightCountDebugDescriptorSetLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo lightCullingPipelineLayoutInfo{};
        lightCullingPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lightCullingPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(lightCountDescriptorSetLayouts.size());
        lightCullingPipelineLayoutInfo.pSetLayouts = lightCountDescriptorSetLayouts.data();
        lightCullingPipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(lighCountPushConstantRanges.size());
        lightCullingPipelineLayoutInfo.pPushConstantRanges = lighCountPushConstantRanges.data();

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &lightCullingPipelineLayoutInfo, nullptr, &lightCountDebugPipelineLayout),
            "[Sumire::HSQMdebugger] Failed to create light culling debug pipeline layout."
        );
    }

    void HQSMdebugger::createLightCountDebugPipeline(VkRenderPass renderPass) {

        PipelineConfigInfo lightCullingDebugPipelineInfo{};
        SumiPipeline::defaultPipelineConfigInfo(lightCullingDebugPipelineInfo);
        SumiPipeline::enableAlphaBlending(lightCullingDebugPipelineInfo);
        lightCullingDebugPipelineInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        lightCullingDebugPipelineInfo.attributeDescriptions.clear();
        lightCullingDebugPipelineInfo.bindingDescriptions.clear();
        lightCullingDebugPipelineInfo.pipelineLayout = lightCountDebugPipelineLayout;
        lightCullingDebugPipelineInfo.renderPass = renderPass;

        lightCountDebugPipeline = std::make_unique<SumiPipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_fs_triangle.vert"),
            SUMIRE_ENGINE_PATH("shaders/high_quality_shadow_mapping/debug/vis_light_count.frag"),
            lightCullingDebugPipelineInfo
        );

    }

}