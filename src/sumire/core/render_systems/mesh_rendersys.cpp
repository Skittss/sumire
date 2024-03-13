#include <sumire/core/render_systems/mesh_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/mesh_rendersys_structs.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	// Note: If any more Frag push constants are needed, create a struct here with 
	//		 SumiMaterial::MaterialPushConstantData as an internal struct.

	MeshRenderSys::MeshRenderSys(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		) : sumiDevice{device} {
		
		// Check physical device can support the size of push constants for this pipeline.
		//  VK min guarantee is 128 bytes, this pipeline targets 256 bytes.
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &deviceProperties);
		uint32_t requiredPushConstantSize = static_cast<uint32_t>(
			sizeof(structs::VertPushConstantData) + sizeof(structs::FragPushConstantData
		));
		if (deviceProperties.limits.maxPushConstantsSize < requiredPushConstantSize) {
			std::runtime_error(
				"Mesh rendering requires at least" + 
				std::to_string(requiredPushConstantSize) +
				" bytes of push constant storage. Physical device used supports only " +
				std::to_string(deviceProperties.limits.maxPushConstantsSize) +
				" bytes."
			);
		}

		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines(renderPass);
	}

	MeshRenderSys::~MeshRenderSys() {
		// TODO: Destroy pipeline at this line??
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	void MeshRenderSys::createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout) {

		VkPushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		vertPushConstantRange.offset = 0;
		vertPushConstantRange.size = sizeof(structs::VertPushConstantData);

		VkPushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragPushConstantRange.offset = sizeof(structs::VertPushConstantData);
		fragPushConstantRange.size = sizeof(structs::FragPushConstantData);

		std::vector<VkPushConstantRange> pushConstantRanges{
			vertPushConstantRange,
			fragPushConstantRange
		};

		matStorageDescriptorLayout = SumiModel::matStorageDescriptorLayout(sumiDevice);
		meshNodeDescriptorLayout = SumiModel::meshNodeDescriptorLayout(sumiDevice);
		matTextureDescriptorLayout = SumiModel::matTextureDescriptorLayout(sumiDevice);
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
			globalDescriptorSetLayout,
			matTextureDescriptorLayout->getDescriptorSetLayout(),
			meshNodeDescriptorLayout->getDescriptorSetLayout(),
			matStorageDescriptorLayout->getDescriptorSetLayout()
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		if (vkCreatePipelineLayout(
				sumiDevice.device(), 
				&pipelineLayoutInfo,
				nullptr, 
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("<MeshRenderSys>: Failed to create pipeline layout.");
		}
	}

	void MeshRenderSys::createPipelines(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "<MeshRenderSys>: Cannot create pipeline before pipeline layout.");

		// Default Pipeline
		PipelineConfigInfo defaultConfig{};
		SumiPipeline::defaultPipelineConfigInfo(defaultConfig);
		defaultConfig.renderPass = renderPass;
		defaultConfig.pipelineLayout = pipelineLayout;
		std::unique_ptr<SumiPipeline> defaultPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/simple_mesh.vert.spv", 
			"shaders/simple_mesh.frag.spv",
			defaultConfig
		);
		pipelines.emplace(
			SumiMaterial::RequiredPipelineType::DEFAULT, std::move(defaultPipeline));

		// TODO: The following pipeline could be created with dynamic state to poke the 
		//		cull-mode register, or as a derivative of the default pipeline. May be faster.

		// Double-sided Pipeline
		PipelineConfigInfo doubleSidedConfig{};
		SumiPipeline::defaultPipelineConfigInfo(doubleSidedConfig);
		doubleSidedConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		doubleSidedConfig.renderPass = renderPass;
		doubleSidedConfig.pipelineLayout = pipelineLayout;
		std::unique_ptr<SumiPipeline> doubleSidedPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/simple_mesh.vert.spv", 
			"shaders/simple_mesh.frag.spv",
			doubleSidedConfig
		);
		pipelines.emplace(
			SumiMaterial::RequiredPipelineType::DOUBLE_SIDED, std::move(doubleSidedPipeline));
	}

	void MeshRenderSys::renderObjects(FrameInfo &frameInfo) {

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&frameInfo.globalDescriptorSet,
			0, nullptr
		);

		for (auto& kv: frameInfo.objects) {
			auto& obj = kv.second;
			
			// Only render objects with a mesh
			if (obj.model == nullptr) continue;

			structs::VertPushConstantData push{};
			push.modelMatrix = obj.transform.modelMatrix();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer, 
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(structs::VertPushConstantData),
				&push
			);

			// TODO: Link animation playback (e.g. index, timer, loop) to UI.
			//		 For now, play all animations, looped.
			// TODO: This update can and should be done on a separate thread.
			//		 Updating joint matrices may need to be made thread safe / double buffered as a result.
			if (obj.model->getAnimationCount() > 0) {
				std::vector<uint32_t> indices(obj.model->getAnimationCount());
				for (uint32_t i = 0; i < indices.size(); i++) indices[i] = i;
				obj.model->updateAnimations(indices, frameInfo.cumulativeFrameTime);
			}
			
			// SumiModel handles the binding of descriptor sets 1-3 and frag push constants
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer, pipelineLayout, pipelines);
		}
	}
}