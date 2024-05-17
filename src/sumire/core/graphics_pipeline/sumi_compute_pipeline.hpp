#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <string>

namespace sumire {

    class SumiComputePipeline {
    public:
        SumiComputePipeline() = default;
        SumiComputePipeline(
            SumiDevice& device,
            const std::string& compShaderPath,
            VkPipelineLayout pipelineLayout
        );
        ~SumiComputePipeline();

        SumiComputePipeline(const SumiComputePipeline&) = delete;
        SumiComputePipeline& operator=(const SumiComputePipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);

    private:
        void createComputePipeline(VkPipelineLayout pipelineLayout);
        void getShaderSources(const std::string& compFilepath);

        std::string compFilePath = "Undefined";

        ShaderSource* compShaderSource = nullptr;

        SumiDevice& sumiDevice;
        VkPipeline computePipeline = VK_NULL_HANDLE;

        static SumiComputePipeline* boundPipeline;

    };

}