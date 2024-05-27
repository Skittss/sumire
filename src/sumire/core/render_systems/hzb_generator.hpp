#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/rendering/geometry/sumi_hzb.hpp>

#include <memory>

namespace sumire {

    class HzbGenerator {
    public:
        HzbGenerator(
            SumiDevice& device, 
            SumiAttachment* zbuffer,
            SumiHZB* hzb
        );
        ~HzbGenerator();

        void generateFullHzbMipChain(VkCommandBuffer commandBuffer);
        void generateShadowTileHzb(VkCommandBuffer commandBuffer);

        void updateDescriptors(SumiAttachment* zbuffer, SumiHZB* hzb);

    private:
        void createZbufferSampler();
        void initDescriptors(SumiAttachment* zbuffer, SumiHZB* hzb);
        void createPipelineLayouts();
        void createPipelines();

        SumiDevice& sumiDevice;

        //std::unique_ptr<SumiHZB> hzb;
        VkExtent2D zbufferResolution;
        VkExtent2D hzbResolution;
        VkSampler zbufferSampler = VK_NULL_HANDLE;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;
        std::unique_ptr<SumiDescriptorSetLayout> descriptorSetLayout;
        VkDescriptorSet descriptorSet;

        std::unique_ptr<SumiComputePipeline> computePipeline;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;


    };

}