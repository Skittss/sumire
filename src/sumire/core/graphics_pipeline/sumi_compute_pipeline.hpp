#pragma once

#include <sumire/core/graphics_pipeline/impl_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <string>

namespace sumire {

    class SumiComputePipeline : public ImplPipeline {
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

        void bind(VkCommandBuffer commandBuffer) override;
        void queuePipelineRecreation() override;

        static void resetBoundPipelineCache() { boundPipeline = nullptr; }

    private:
        void createComputePipeline(VkPipeline* pipeline);
        void destroyComputePipeline();
        void swapNewComputePipeline();
        void getShaderSources(const std::string& compFilepath);

        std::string compFilePath = "Undefined";

        ShaderSource* compShaderSource = nullptr;
        bool needsNewPipelineSwap = false;

        SumiDevice& sumiDevice;
        VkPipeline computePipeline             = VK_NULL_HANDLE;
        VkPipeline newComputePipeline          = VK_NULL_HANDLE;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

        static SumiComputePipeline* boundPipeline;

    };

}