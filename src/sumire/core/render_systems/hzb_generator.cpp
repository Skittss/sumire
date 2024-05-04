#include <sumire/core/render_systems/hzb_generator.hpp>
#include <sumire/core/render_systems/data_structs/hzb_generator_structs.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {

    HzbGenerator::HzbGenerator(
        SumiDevice& device,
        SumiAttachment* zbuffer
    ) : sumiDevice{ device } {
        createZbufferSampler();
        initDescriptors(zbuffer);
        createPipelineLayouts();
        createPipelines();
    }

    HzbGenerator::~HzbGenerator() {
        vkDestroyPipelineLayout(sumiDevice.device(), computePipelineLayout, nullptr);
        vkDestroySampler(sumiDevice.device(), zbufferSampler, nullptr);
    }

    void HzbGenerator::generateSingleHzbMip(VkCommandBuffer commandBuffer) {
        computePipeline->bind(commandBuffer);

        structs::hzbPush push{};
        push.resolution = glm::vec2(
            zbufferResolution.width, zbufferResolution.height);

        vkCmdPushConstants(
            commandBuffer,
            computePipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(structs::hzbPush),
            &push
        );

        std::array<VkDescriptorSet, 1> hzbDescriptors{
            descriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipelineLayout,
            0, static_cast<uint32_t>(hzbDescriptors.size()),
            hzbDescriptors.data(),
            0, nullptr
        );

        uint32_t groupSizeX = zbufferResolution.width  / 8;
        uint32_t groupSizeY = zbufferResolution.height / 8;

        vkCmdDispatch(commandBuffer, groupSizeX, groupSizeY, 1);
    }

    void HzbGenerator::createZbufferSampler() {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Clamp to edge to not affect downsample min 
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
            vkCreateSampler(sumiDevice.device(), &samplerCreateInfo, nullptr, &zbufferSampler),
            "[Sumire::HzbGenerator] Failed to create zbuffer sampler."
        );
    }

    void HzbGenerator::initDescriptors(SumiAttachment* zbuffer) {
        assert(zbuffer && "zbuffer not provided.");
        assert(zbufferSampler && "zbuffer sampler not initialized.");

        descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
            .build();

        descriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

        zbufferResolution = zbuffer->getExtent();

        VkDescriptorImageInfo hzbImageStoreDescriptor{};
        hzbImageStoreDescriptor.sampler = zbufferSampler;
        hzbImageStoreDescriptor.imageView = zbuffer->getImageView();
        hzbImageStoreDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        SumiDescriptorWriter(*descriptorSetLayout, *descriptorPool)
            .writeImage(0, &hzbImageStoreDescriptor)
            .build(descriptorSet);
    }

    void HzbGenerator::updateDescriptors(SumiAttachment* zbuffer) {
        assert(zbuffer && "zbuffer not provided.");
        assert(zbufferSampler && "zbuffer sampler not initialized.");

        zbufferResolution = zbuffer->getExtent();

        VkDescriptorImageInfo hzbImageStoreDescriptor{};
        hzbImageStoreDescriptor.sampler = zbufferSampler;
        hzbImageStoreDescriptor.imageView = zbuffer->getImageView();
        hzbImageStoreDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*descriptorSetLayout, *descriptorPool)
            .writeImage(0, &hzbImageStoreDescriptor)
            .overwrite(descriptorSet);
    }

    void HzbGenerator::createPipelineLayouts() {

        VkPushConstantRange computeHdrImagePushRange{};
        computeHdrImagePushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        computeHdrImagePushRange.offset = 0;
        computeHdrImagePushRange.size = sizeof(structs::hzbPush);

        std::vector<VkDescriptorSetLayout> computeDescriptorSetLayouts{
            descriptorSetLayout->getDescriptorSetLayout()
        };

        std::vector<VkPushConstantRange> computePushConstantRanges{
            computeHdrImagePushRange
        };

        VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
        computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(computeDescriptorSetLayouts.size());
        computePipelineLayoutInfo.pSetLayouts = computeDescriptorSetLayouts.data();
        computePipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(computePushConstantRanges.size());
        computePipelineLayoutInfo.pPushConstantRanges = computePushConstantRanges.data();

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &computePipelineLayoutInfo, nullptr, &computePipelineLayout),
            "[Sumire::PostProcessor] Failed to create compute pipeline layout."
        );
    }

    void HzbGenerator::createPipelines() {
        assert(computePipelineLayout != VK_NULL_HANDLE
            && "Cannot create pipelines when pipeline layout is VK_NULL_HANDLE.");

        computePipeline = std::make_unique<SumiComputePipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/hzb/gen_single_hzb.comp.spv"),
            computePipelineLayout
        );
    }

}