#pragma once

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
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpass = 0;
    };

    class SumiPipeline {
    public:
        SumiPipeline() = default;
        SumiPipeline(
            SumiDevice& device, 
            const std::string& vertFilepath, 
            const std::string& fragFilepath, 
            const PipelineConfigInfo& configInfo
        );
        ~SumiPipeline();

        SumiPipeline(const SumiPipeline&) = delete;
        SumiPipeline& operator=(const SumiPipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);
        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void enableAlphaBlending(PipelineConfigInfo& configInfo);

    private:
        void createGraphicsPipeline(const PipelineConfigInfo& configInfo);
        void getShaderSources(const std::string& vertFilepath, const std::string& fragFilepath);

        std::string vertFilePath = "Undefined";
        std::string fragFilePath = "Undefined";

        ShaderSource* vertShaderSource = nullptr;
        ShaderSource* fragShaderSource = nullptr;

        SumiDevice& sumiDevice;
        VkPipeline graphicsPipeline     = VK_NULL_HANDLE;

        static SumiPipeline* boundPipeline;

    };
}