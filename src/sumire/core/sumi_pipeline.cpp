#include <sumire/core/sumi_pipeline.hpp>

#include <sumire/core/sumi_model.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>

// Deal with cmake building to ./build instead of ./
#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace sumire {

	SumiPipeline::SumiPipeline(
		SumiDevice& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo
	) : sumiDevice{ device } {
		createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
	}

	SumiPipeline::~SumiPipeline() {
		vkDestroyShaderModule(sumiDevice.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(sumiDevice.device(), fragShaderModule, nullptr);
		vkDestroyPipeline(sumiDevice.device(), graphicsPipeline, nullptr);
	}

	std::vector<char> SumiPipeline::readFile(const std::string& filepath) {
		std::string enginePath = ENGINE_DIR + filepath;
		std::ifstream file{ enginePath, std::ios::ate | std::ios::binary };

		if (!file.is_open()) {
			throw std::runtime_error("Could not open file: " + enginePath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	void SumiPipeline::createGraphicsPipeline(
		const std::string& vertFilepath, 
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo
	) {
		assert(
			configInfo.pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: pipelineLayout is not provided in configInfo");
		assert(
			configInfo.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: renderPass is not provided in configInfo");

		auto vertCode = readFile(vertFilepath);
		auto fragCode = readFile(fragFilepath);

		createShaderModule(vertCode, &vertShaderModule);
		createShaderModule(fragCode, &fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main"; // Name of entry function in vert shader.
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main"; // Name of entry function in vert shader.
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		auto bindingDescriptions = SumiModel::Vertex::getBindingDescriptions();
		auto attributeDescriptions = SumiModel::Vertex::getAttributeDescriptions();
		// How to interpret input vertex buffer data
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// For now, supply no data
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2; // How many programmable stages we use
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo; // If set, dynamic functionality without recreating pipeline e.g. viewport size.

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		// Performance params for pipeline creation
		//  It can be faster to create a pipeline derrived from an existing one.
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(
				sumiDevice.device(),
				VK_NULL_HANDLE,
				1,
				&pipelineInfo,
				nullptr,
				&graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}
	}

	void SumiPipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(sumiDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module");
		}
	}

	void SumiPipeline::bind(VkCommandBuffer commandBuffer) {
		// Pipeline types are graphics, compute and raytracing.
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}
	void SumiPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
		// NOTE: Reading the Vk Documentation is VERY IMPORTANT for configuring configs as you desire.

		// Triangle list input assembly.
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive typing
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // Enable = allow terminator chr to break triangle strips

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		// Fragment config
		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE; // Enable = Clamp on z values to [0, 1]
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // Enable = only use first few stages of pipeline. (i.e. no raster)
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // Triangle culling. (NONE) to not accidentally cull right now.
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Determine which to cull via winding order
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

		// MSAA - I think
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

		// Color blending
		//	Combining colours in the framebuffer - overlapping triangles will return multiple colours;
		//	We blend against the current framebuffer colour to get a single output colour for our pixel
		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};  // Optional
		configInfo.depthStencilInfo.back = {};   // Optional

		configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;
	}

}