#pragma once

#include <sumire/core/graphics_pipeline/impl_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <string>
#include <vector>

namespace sumire {

    struct PipelineConfigInfo {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpass = 0;
    };

    class SumiPipeline : public ImplPipeline {
    public:
        SumiPipeline() = default;
        SumiPipeline(
            SumiDevice& device, 
            const std::string& vertFilepath, 
            const std::string& fragFilepath, 
            PipelineConfigInfo configInfo
        );
        ~SumiPipeline();

        SumiPipeline(const SumiPipeline&) = delete;
        SumiPipeline& operator=(const SumiPipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer) override;
        void queuePipelineRecreation() override;
        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);

        static void resetBoundPipelineCache() { boundPipeline = nullptr; }

    private:
        void createGraphicsPipeline(VkPipeline* pipeline);
        void destroyGraphicsPipeline();
        void swapNewGraphicsPipeline();
        void getShaderSources(const std::string& vertFilepath, const std::string& fragFilepath);

        std::string vertFilePath = "Undefined";
        std::string fragFilePath = "Undefined";

        ShaderSource* vertShaderSource = nullptr;
        ShaderSource* fragShaderSource = nullptr;
        bool needsNewPipelineSwap = false;

        SumiDevice& sumiDevice;
        VkPipeline graphicsPipeline    = VK_NULL_HANDLE;
        VkPipeline newGraphicsPipeline = VK_NULL_HANDLE;
        PipelineConfigInfo configInfo;

        static SumiPipeline* boundPipeline;

    };
}