#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>

#include <sumire/core/shaders/shader_manager.hpp>
#include <sumire/core/models/vertex.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace sumire {

    // Initialize reference to currently bound pipeline as nullptr
    SumiPipeline* SumiPipeline::boundPipeline = nullptr;

    SumiPipeline::SumiPipeline(
        SumiDevice& device,
        const std::string& vertFilepath,
        const std::string& fragFilepath,
        PipelineConfigInfo configInfo
    ) : sumiDevice{ device }, 
        vertFilePath{ vertFilePath }, 
        fragFilePath{fragFilepath},
        configInfo{ configInfo }
    {
        getShaderSources(vertFilepath, fragFilepath);
        createGraphicsPipeline(&graphicsPipeline);
    }

    SumiPipeline::~SumiPipeline() {
        destroyGraphicsPipeline();

        // nullify binding reference if this pipeline is currently bound.
        if (boundPipeline == this) resetBoundPipelineCache();
    }

    void SumiPipeline::createGraphicsPipeline(VkPipeline* pipeline) {
        assert(configInfo.pipelineLayout != VK_NULL_HANDLE
            && "Cannot create graphics pipeline: pipelineLayout is not provided in configInfo");
        assert(configInfo.renderPass != VK_NULL_HANDLE
            && "Cannot create graphics pipeline: renderPass is not provided in configInfo");

        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module               = vertShaderSource->getShaderModule();
        shaderStages[0].pName               = "main"; // Name of entry function in vert shader.
        shaderStages[0].flags               = 0;
        shaderStages[0].pNext               = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        shaderStages[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module               = fragShaderSource->getShaderModule();
        shaderStages[1].pName               = "main"; // Name of entry function in frag shader.
        shaderStages[1].flags               = 0;
        shaderStages[1].pNext               = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        // Populate sub-structs that require pointerized data.
        //   All the necessary data is contained within configInfo as other memebers so that
        //   We don't have hanging pointers.

        // Input vertex data
        auto& bindingDescriptions   = configInfo.bindingDescriptions;
        auto& attributeDescriptions = configInfo.attributeDescriptions;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
        vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions.data();
        
        // Color blending
        VkPipelineColorBlendStateCreateInfo colorBlendInfo = configInfo.colorBlendInfo; // maybe ref this?
        colorBlendInfo.attachmentCount = static_cast<uint32_t>(configInfo.colorBlendAttachments.size());
        colorBlendInfo.pAttachments    = configInfo.colorBlendAttachments.data();

        // Dynamic state info
        VkPipelineDynamicStateCreateInfo dynamicStateInfo = configInfo.dynamicStateInfo; // maybe ref this?
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        dynamicStateInfo.pDynamicStates    = configInfo.dynamicStateEnables.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2; // How many programmable stages we use
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState      = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState   = &configInfo.multisampleInfo;
        pipelineInfo.pColorBlendState    = &colorBlendInfo;
        pipelineInfo.pDepthStencilState  = &configInfo.depthStencilInfo;
        pipelineInfo.pDepthStencilState  = &configInfo.depthStencilInfo;
        pipelineInfo.pDynamicState       = &dynamicStateInfo;

        pipelineInfo.layout     = configInfo.pipelineLayout;
        pipelineInfo.renderPass = configInfo.renderPass;
        pipelineInfo.subpass    = configInfo.subpass;

        // Performance params for pipeline creation
        //  It can be faster to create a pipeline derrived from an existing one.
        pipelineInfo.basePipelineIndex  = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CHECK_SUCCESS(
            vkCreateGraphicsPipelines(
                sumiDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline),
            "[Sumire::SumiPipeline] Failed to create graphics pipeline."
        );
    }

    void SumiPipeline::destroyGraphicsPipeline() {
        vkDestroyPipeline(sumiDevice.device(), graphicsPipeline, nullptr);
    }

    void SumiPipeline::swapNewGraphicsPipeline() {
        assert(newGraphicsPipeline != VK_NULL_HANDLE && "New graphics pipeline not available");

        // Don't like this wait idle but not the biggest deal at the moment
        vkDeviceWaitIdle(sumiDevice.device());
        destroyGraphicsPipeline();
        graphicsPipeline = newGraphicsPipeline;
        newGraphicsPipeline = VK_NULL_HANDLE;
        needsNewPipelineSwap = false;
    }

    void SumiPipeline::getShaderSources(
        const std::string& vertFilepath,
        const std::string& fragFilepath
    ) {
        ShaderManager* shaderManager = sumiDevice.shaderManager();
        vertShaderSource = shaderManager->requestShaderSource(vertFilepath, this);
        fragShaderSource = shaderManager->requestShaderSource(fragFilepath, this);
    }

    void SumiPipeline::bind(VkCommandBuffer commandBuffer) {
        if (needsNewPipelineSwap) swapNewGraphicsPipeline();
        // TODO: This pipeline switching optimization is not perfect -
        //       We currently don't check if pipelines are semantically the same but under different objects.
        //       This mean pipelines will always switch between render systems, etc.
        //		 This could be fixed by comparing a hash of the pipeline state instead of a reference.
        if (boundPipeline != this) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            boundPipeline = this;
        }
    }

    void SumiPipeline::queuePipelineRecreation() {
        createGraphicsPipeline(&newGraphicsPipeline);
        needsNewPipelineSwap = true;
    }

    void SumiPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {

        // Triangle list input assembly.
        configInfo.inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.inputAssemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive typing
        configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // Enable = allow terminator chr to break triangle strips

        configInfo.viewportInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configInfo.viewportInfo.viewportCount = 1;
        configInfo.viewportInfo.pViewports    = nullptr;
        configInfo.viewportInfo.scissorCount  = 1;
        configInfo.viewportInfo.pScissors     = nullptr;

        // Fragment config
        configInfo.rasterizationInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configInfo.rasterizationInfo.depthClampEnable        = VK_FALSE; // Enable = Clamp on z values to [0, 1]
        configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // Enable = only use first few stages of pipeline. (i.e. no raster)
        configInfo.rasterizationInfo.polygonMode             = VK_POLYGON_MODE_FILL;
        configInfo.rasterizationInfo.lineWidth               = 1.0f;
        configInfo.rasterizationInfo.cullMode                = VK_CULL_MODE_BACK_BIT; // Back-face triangle culling
        configInfo.rasterizationInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Determine which to cull via winding order
        configInfo.rasterizationInfo.depthBiasEnable         = VK_FALSE;
        configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
        configInfo.rasterizationInfo.depthBiasClamp          = 0.0f;
        configInfo.rasterizationInfo.depthBiasSlopeFactor    = 0.0f;

        // MSAA - I think
        configInfo.multisampleInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configInfo.multisampleInfo.sampleShadingEnable   = VK_FALSE;
        configInfo.multisampleInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        configInfo.multisampleInfo.minSampleShading      = 1.0f;    
        configInfo.multisampleInfo.pSampleMask           = nullptr; 
        configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        configInfo.multisampleInfo.alphaToOneEnable      = VK_FALSE;

        // Color blending
        //	Combining colours in the framebuffer - overlapping triangles will return multiple colours;
        //	We blend against the current framebuffer colour to get a single output colour for our pixel.
        VkPipelineColorBlendAttachmentState defaultColorBlendAttachment{};
        defaultColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        defaultColorBlendAttachment.blendEnable         = VK_FALSE;
        defaultColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
        defaultColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        defaultColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;     
        defaultColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
        defaultColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        defaultColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;     
        configInfo.colorBlendAttachments = { defaultColorBlendAttachment };

        configInfo.colorBlendInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configInfo.colorBlendInfo.logicOpEnable     = VK_FALSE;
        configInfo.colorBlendInfo.logicOp           = VK_LOGIC_OP_COPY;
        configInfo.colorBlendInfo.attachmentCount   = static_cast<uint32_t>(configInfo.colorBlendAttachments.size());
        configInfo.colorBlendInfo.pAttachments      = configInfo.colorBlendAttachments.data();
        configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

        configInfo.depthStencilInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.depthStencilInfo.depthTestEnable       = VK_TRUE;
        configInfo.depthStencilInfo.depthWriteEnable      = VK_TRUE;
        configInfo.depthStencilInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
        configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.minDepthBounds        = 0.0f; 
        configInfo.depthStencilInfo.maxDepthBounds        = 1.0f; 
        configInfo.depthStencilInfo.stencilTestEnable     = VK_FALSE;
        configInfo.depthStencilInfo.front                 = {};  
        configInfo.depthStencilInfo.back                  = {};   

        configInfo.dynamicStateEnables                = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        configInfo.dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        configInfo.dynamicStateInfo.pDynamicStates    = configInfo.dynamicStateEnables.data();
        configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        configInfo.dynamicStateInfo.flags             = 0;

        configInfo.bindingDescriptions = Vertex::getBindingDescriptions();
        configInfo.attributeDescriptions = Vertex::getAttributeDescriptions();
    }

    void SumiPipeline::enableAlphaBlending(PipelineConfigInfo &configInfo) {
        for (auto& colorBlendAttachment : configInfo.colorBlendAttachments) {
            colorBlendAttachment.blendEnable         = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        }
    }

}