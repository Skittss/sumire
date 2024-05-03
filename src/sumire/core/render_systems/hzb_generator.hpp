#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/rendering/sumi_hzb.hpp>

#include <memory>

namespace sumire {

    class HzbGenerator {
    public:
        HzbGenerator(SumiDevice& device, SumiAttachment* zbuffer);
        ~HzbGenerator();

        void generateFullMipChain();
        void generateSingleMip();

    private:
        void createZbufferSampler();
        void initDescriptors(SumiAttachment* zbuffer);
        void updateDescriptors(SumiAttachment* zbuffer);
        void createPipelineLayouts();
        void createPipelines();

        SumiDevice& sumiDevice;

        //std::unique_ptr<SumiHZB> hzb;
        VkExtent2D zbufferResolution;
        VkSampler zbufferSampler = VK_NULL_HANDLE;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;
        std::unique_ptr<SumiDescriptorSetLayout> descriptorSetLayout;
        VkDescriptorSet descriptorSet;

        std::unique_ptr<SumiComputePipeline> computePipeline;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;


    };

}