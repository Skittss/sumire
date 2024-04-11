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
		createComputePipeline(compFilepath, pipelineLayout);
	}

	SumiComputePipeline::~SumiComputePipeline() {
		vkDestroyPipeline(sumiDevice.device(), computePipeline, nullptr);
		vkDestroyShaderModule(sumiDevice.device(), compShaderModule, nullptr);
	}

	void SumiComputePipeline::createComputePipeline(
		const std::string& compFilepath, VkPipelineLayout pipelineLayout
	) {
		auto compCode = readFile(compFilePath);
		createShaderModule(compCode, &compShaderModule);

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageInfo.module = compShaderModule;
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

	std::vector<char> SumiComputePipeline::readFile(const std::string& filepath) {
		std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

		if (!file.is_open()) {
			throw std::runtime_error("[Sumire::SumiComputePipeline] Could not open file: " + filepath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	void SumiComputePipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VK_CHECK_SUCCESS(
			vkCreateShaderModule(sumiDevice.device(), &createInfo, nullptr, shaderModule),
			"[Sumire::SumiComputePipeline] Failed to create shader module."
		);
	}

	void SumiComputePipeline::bind(VkCommandBuffer commandBuffer) {
		//if (boundPipeline != this) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		//	boundPipeline = this;
		//}
	}
}