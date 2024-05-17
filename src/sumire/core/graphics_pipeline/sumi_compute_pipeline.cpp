#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace sumire {

    SumiComputePipeline* SumiComputePipeline::boundPipeline = nullptr;

    SumiComputePipeline::SumiComputePipeline(
        SumiDevice& device,
        const std::string& compFilepath,
        VkPipelineLayout pipelineLayout
    ) : sumiDevice{ device }, compFilePath{ compFilepath } {
        getShaderSources(compFilepath);
        createComputePipeline(pipelineLayout);
    }

    SumiComputePipeline::~SumiComputePipeline() {
        vkDestroyPipeline(sumiDevice.device(), computePipeline, nullptr);
    }

    void SumiComputePipeline::createComputePipeline(VkPipelineLayout pipelineLayout) {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = compShaderSource->getShaderModule();
        shaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout;

        VK_CHECK_SUCCESS(
            vkCreateComputePipelines(
                sumiDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline),
            "[Sumire::SumiComputePipeline] Failed to create compute pipeline."
        );
    }

    void SumiComputePipeline::getShaderSources(const std::string& compFilepath) {
        compShaderSource = sumiDevice.shaderManager()->requestShaderSource(compFilepath, this);
    }

    void SumiComputePipeline::bind(VkCommandBuffer commandBuffer) {
        //if (boundPipeline != this) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        //	boundPipeline = this;
        //}
    }
}