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

        void createComputePipeline(
            const std::string& compFilepath, VkPipelineLayout pipelineLayout);

        static std::vector<char> readFile(const std::string& filepath);
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        std::string compFilePath = "Undefined";

        SumiDevice& sumiDevice;
        VkPipeline computePipeline;
        VkShaderModule compShaderModule;

        static SumiComputePipeline* boundPipeline;

    };

}