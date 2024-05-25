#include <sumire/core/render_systems/deferred_mesh_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/deferred_mesh_rendersys_structs.hpp>

#include <sumire/util/sumire_engine_path.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <sumire/core/flags/sumi_pipeline_state_flags.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

    DeferredMeshRenderSys::DeferredMeshRenderSys(
        SumiDevice& device,
        SumiGbuffer* gbuffer,
        VkRenderPass gbufferFillRenderPass,
        uint32_t gbufferFillSubpassIdx,
        VkRenderPass gbufferResolveRenderPass,
        uint32_t gbufferResolveSubpassIdx,
        VkDescriptorSetLayout globalDescriptorSetLayout
    ) : sumiDevice{ device } {
        
        // Check physical device can support the size of push constants for this pipeline.
        //  VK min guarantee is 128 bytes, this pipeline targets 256 bytes.
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &deviceProperties);
        uint32_t requiredPushConstantSize = static_cast<uint32_t>(
            sizeof(structs::VertPushConstantData) + sizeof(structs::FragPushConstantData
        ));
        if (deviceProperties.limits.maxPushConstantsSize < requiredPushConstantSize) {
            throw std::runtime_error(
                "Mesh rendering requires at least" + 
                std::to_string(requiredPushConstantSize) +
                " bytes of push constant storage. Physical device used supports only " +
                std::to_string(deviceProperties.limits.maxPushConstantsSize) +
                " bytes."
            );
        }

        createGbufferSampler();
        initResolveDescriptors(gbuffer);
        createPipelineLayouts(globalDescriptorSetLayout);
        createPipelines(
            gbufferFillRenderPass, gbufferFillSubpassIdx, 
            gbufferResolveRenderPass, gbufferResolveSubpassIdx
        );
    }

    DeferredMeshRenderSys::~DeferredMeshRenderSys() {
        vkDestroyPipelineLayout(sumiDevice.device(), resolvePipelineLayout, nullptr);
        vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
        vkDestroySampler(sumiDevice.device(), gbufferSampler, nullptr);
    }

    void DeferredMeshRenderSys::createGbufferSampler() {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.anisotropyEnable = VK_FALSE;
        samplerCreateInfo.maxAnisotropy = 0.0f;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;

        VK_CHECK_SUCCESS(
            vkCreateSampler(sumiDevice.device(), &samplerCreateInfo, nullptr, &gbufferSampler),
            "[Sumire::DeferredMeshRenderSys] Failed to create gbuffer sampler."
        );
    }

    void DeferredMeshRenderSys::initResolveDescriptors(SumiGbuffer* gbuffer) {
        resolveDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(4)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)
            .build();

        resolveDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        // Gbuffer attachment descriptors
        VkDescriptorImageInfo positionDescriptor{};
        positionDescriptor.sampler = gbufferSampler;
        positionDescriptor.imageView = gbuffer->positionAttachment()->getImageView();
        positionDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo normalDescriptor{};
        normalDescriptor.sampler = gbufferSampler;
        normalDescriptor.imageView = gbuffer->normalAttachment()->getImageView();
        normalDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo albedoDescriptor{};
        albedoDescriptor.sampler = gbufferSampler;
        albedoDescriptor.imageView = gbuffer->albedoAttachment()->getImageView();
        albedoDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo aoMetalRoughEmissive{};
        aoMetalRoughEmissive.sampler = gbufferSampler;
        aoMetalRoughEmissive.imageView = gbuffer->aoMetalRoughEmissiveAttachment()->getImageView();
        aoMetalRoughEmissive.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*resolveDescriptorSetLayout, *resolveDescriptorPool)
            .writeImage(0, &positionDescriptor)
            .writeImage(1, &normalDescriptor)
            .writeImage(2, &albedoDescriptor)
            .writeImage(3, &aoMetalRoughEmissive)
            .build(resolveDescriptorSet);
    }

    void DeferredMeshRenderSys::updateResolveDescriptors(SumiGbuffer* gbuffer) {
        // Gbuffer attachment descriptors
        VkDescriptorImageInfo positionDescriptor{};
        positionDescriptor.sampler = gbufferSampler;
        positionDescriptor.imageView = gbuffer->positionAttachment()->getImageView();
        positionDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo normalDescriptor{};
        normalDescriptor.sampler = gbufferSampler;
        normalDescriptor.imageView = gbuffer->normalAttachment()->getImageView();
        normalDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo albedoDescriptor{};
        albedoDescriptor.sampler = gbufferSampler;
        albedoDescriptor.imageView = gbuffer->albedoAttachment()->getImageView();
        albedoDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo aoMetalRoughEmissive{};
        aoMetalRoughEmissive.sampler = gbufferSampler;
        aoMetalRoughEmissive.imageView = gbuffer->aoMetalRoughEmissiveAttachment()->getImageView();
        aoMetalRoughEmissive.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*resolveDescriptorSetLayout, *resolveDescriptorPool)
            .writeImage(0, &positionDescriptor)
            .writeImage(1, &normalDescriptor)
            .writeImage(2, &albedoDescriptor)
            .writeImage(3, &aoMetalRoughEmissive)
            .overwrite(resolveDescriptorSet);
    }

    void DeferredMeshRenderSys::createPipelineLayouts(VkDescriptorSetLayout globalDescriptorSetLayout) {
        // Gbuffer Pipelines (Mesh rendering)
        VkPushConstantRange vertPushConstantRange{};
        vertPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vertPushConstantRange.offset = 0;
        vertPushConstantRange.size = sizeof(structs::VertPushConstantData);

        VkPushConstantRange fragPushConstantRange{};
        fragPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragPushConstantRange.offset = sizeof(structs::VertPushConstantData);
        fragPushConstantRange.size = sizeof(structs::FragPushConstantData);

        std::vector<VkPushConstantRange> pushConstantRanges{
            vertPushConstantRange,
            fragPushConstantRange
        };

        matStorageDescriptorLayout = SumiModel::matStorageDescriptorLayout(sumiDevice);
        meshNodeDescriptorLayout = SumiModel::meshNodeDescriptorLayout(sumiDevice);
        matTextureDescriptorLayout = SumiModel::matTextureDescriptorLayout(sumiDevice);
        
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            globalDescriptorSetLayout,
            matTextureDescriptorLayout->getDescriptorSetLayout(),
            meshNodeDescriptorLayout->getDescriptorSetLayout(),
            matStorageDescriptorLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
            "[Sumire::DeferredMeshRenderSys] Failed to create gbuffer rendering pipeline layout."
        );

        VkPushConstantRange resolvePushConstantRange{};
        resolvePushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        resolvePushConstantRange.offset = 0;
        resolvePushConstantRange.size = sizeof(structs::CompositePushConstantData);

        // Composite Pipeline (Lighting)
        std::vector<VkDescriptorSetLayout> resolveDescriptorSetLayouts{
            globalDescriptorSetLayout,
            resolveDescriptorSetLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo resolvePipelineLayoutInfo{};
        resolvePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        resolvePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(resolveDescriptorSetLayouts.size());
        resolvePipelineLayoutInfo.pSetLayouts = resolveDescriptorSetLayouts.data();
        resolvePipelineLayoutInfo.pushConstantRangeCount = 1;
        resolvePipelineLayoutInfo.pPushConstantRanges = &resolvePushConstantRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &resolvePipelineLayoutInfo, nullptr, &resolvePipelineLayout),
            "[Sumire::DeferredMeshRenderSys] Failed to create lighting composition pipeline layout."
        );
    }

    void DeferredMeshRenderSys::createPipelines(
        VkRenderPass fillRenderPass,
        uint32_t fillSubpassIdx,
        VkRenderPass resolveRenderPass,
        uint32_t resolveSubpassIdx
    ) {
        assert(pipelineLayout != VK_NULL_HANDLE && resolvePipelineLayout != VK_NULL_HANDLE
            && "[Sumire::DeferredMeshRenderSys]: Cannot create pipelines before pipeline layouts.");

        // TODO: This pipeline creation step should be moved to a common location so that it can be
        //		 re-used by different render systems. This would also allow for efficient caching
        //		 of these pipelines (i.e. do not recreate if it already exists.)
        // TODO: For now, we can generate all permutations of required pipelines during initialization
        //		 so long as the number of flags remains small. If/when it gets larger, we should generate the
        //		 required pipelines at runtime, with (runtime) caching, and generate a common (serialized) 
        //		 pipeline cache that can load up frequently used pipelines from disk.

        // ---- Gbuffer Pipeline - 5 Color attachments -----------------------------------------------------------

        // Default pipeline config used as base of permutations
        PipelineConfigInfo defaultConfig{};
        SumiPipeline::defaultPipelineConfigInfo(defaultConfig);
        defaultConfig.renderPass = fillRenderPass;
        defaultConfig.subpass = fillSubpassIdx;
        defaultConfig.pipelineLayout = pipelineLayout;

        // Modify color blending for 5 channels as we have 5 colour outputs for deferred rendering.
        // All non-color channels should not alpha blend, but we should enable it for color channels.
        VkPipelineColorBlendAttachmentState noAlphaBlend{};
        noAlphaBlend.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        noAlphaBlend.blendEnable = VK_FALSE;
        noAlphaBlend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        noAlphaBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        noAlphaBlend.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
        noAlphaBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        noAlphaBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        noAlphaBlend.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

        VkPipelineColorBlendAttachmentState alphaBlend{};
        alphaBlend.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        alphaBlend.blendEnable = VK_TRUE;
        alphaBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        alphaBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        alphaBlend.colorBlendOp = VK_BLEND_OP_ADD;
        alphaBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        alphaBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        alphaBlend.alphaBlendOp = VK_BLEND_OP_ADD;

        defaultConfig.colorBlendAttachments.clear();
        defaultConfig.colorBlendAttachments = {
            alphaBlend,   // Swapchain Col
            noAlphaBlend, // Position
            noAlphaBlend, // Normal
            alphaBlend,   // Albedo
            noAlphaBlend  // Depth
        };
        //std::array<VkPipelineColorBlendAttachmentState, 5> colorBlendAttachments{
        //    alphaBlend,   // Swapchain Col
        //    noAlphaBlend, // Position
        //    noAlphaBlend, // Normal
        //    alphaBlend,   // Albedo
        //    noAlphaBlend  // Depth
        //};

        //defaultConfig.colorBlendInfo.pAttachments = colorBlendAttachments.data();
        //defaultConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());

        std::string defaultVertShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill.vert");
        std::string defaultFragShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill.frag");

        // All permutations (including default)
        const uint32_t pipelinePermutations = 2 * SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_HIGHEST;
        for (uint32_t i = 0; i < pipelinePermutations; i++) {
            SumiPipelineStateFlags permutationFlags = static_cast<SumiPipelineStateFlags>(i);
            PipelineConfigInfo permutationConfig = defaultConfig;
            std::string permutationVertShader = defaultVertShader;
            std::string permutationFragShader = defaultFragShader;

            // Deal with each bit flag
            if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_UNLIT_BIT) {
                permutationVertShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill_unlit.vert");
                permutationFragShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill_unlit.frag");
            }
            if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_DOUBLE_SIDED_BIT)
                permutationConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

            // Create pipeline and map it
            std::unique_ptr<SumiPipeline> permutationPipeline = std::make_unique<SumiPipeline>(
                sumiDevice,
                permutationVertShader, 
                permutationFragShader,
                permutationConfig
            );
            pipelines.emplace(permutationFlags, std::move(permutationPipeline));
        }

        // ---- Resolve pipeline ---------------------------------------------------------------------------------
        PipelineConfigInfo resolvePipelineConfig{};
        SumiPipeline::defaultPipelineConfigInfo(resolvePipelineConfig);
        SumiPipeline::enableAlphaBlending(resolvePipelineConfig);
        resolvePipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        resolvePipelineConfig.attributeDescriptions.clear();
        resolvePipelineConfig.bindingDescriptions.clear();
        resolvePipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        resolvePipelineConfig.renderPass = resolveRenderPass;
        resolvePipelineConfig.subpass = resolveSubpassIdx;
        resolvePipelineConfig.pipelineLayout = resolvePipelineLayout;

        resolvePipeline = std::make_unique<SumiPipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_resolve.vert"),
            SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_resolve.frag"),
            resolvePipelineConfig
        );
    }

    void DeferredMeshRenderSys::resolveGbuffer(VkCommandBuffer commandBuffer, FrameInfo &frameInfo) {

        // Bind composite pipeline
        resolvePipeline->bind(commandBuffer);

        structs::CompositePushConstantData push{};
        push.nLights = 1;

        vkCmdPushConstants(
            commandBuffer,
            resolvePipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(structs::CompositePushConstantData),
            &push
        );

        std::array<VkDescriptorSet, 2> resolveDescriptors{
            frameInfo.globalDescriptorSet,
            resolveDescriptorSet
        };

        // Bind descriptor sets (Gbuffer)
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resolvePipelineLayout,
            0, static_cast<uint32_t>(resolveDescriptors.size()),
            resolveDescriptors.data(),
            0, nullptr
        );

        // Composite with a FS quad
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void DeferredMeshRenderSys::fillGbuffer(VkCommandBuffer commandBuffer, FrameInfo &frameInfo) {

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

        for (auto& kv: frameInfo.objects) {
            auto& obj = kv.second;
            
            // Only render objects with a mesh
            if (obj.model == nullptr) continue;

            structs::VertPushConstantData push{};
            push.modelMatrix = obj.transform.modelMatrix();
            push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants(
                commandBuffer, 
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(structs::VertPushConstantData),
                &push
            );

            // TODO: Link animation playback (e.g. index, timer, loop) to UI.
            //		 For now, play all animations, looped.
            // TODO: This update can and should be done on a separate thread.
            //		 Updating joint matrices may need to be made thread safe / double buffered as a result.
            if (obj.model->getAnimationCount() > 0) {
                std::vector<uint32_t> indices(obj.model->getAnimationCount());
                for (uint32_t i = 0; i < indices.size(); i++) indices[i] = i;
                obj.model->updateAnimations(indices, frameInfo.cumulativeFrameTime);
            }
            
            // SumiModel handles the binding of descriptor sets 1-3 and frag push constants
            obj.model->bind(commandBuffer);
            // Each draw command may need a different pipeline, so the model draw binds pipelines at call time.
            obj.model->draw(commandBuffer, pipelineLayout, pipelines);
        }
    }
}