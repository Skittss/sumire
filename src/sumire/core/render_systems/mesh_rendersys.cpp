#include <sumire/core/render_systems/mesh_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/mesh_rendersys_structs.hpp>

#include <sumire/util/sumire_engine_path.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <sumire/core/flags/sumi_pipeline_state_flags.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	MeshRenderSys::MeshRenderSys(
		SumiDevice& device, 
		VkRenderPass renderPass, 
		uint32_t subpassIdx,
		VkDescriptorSetLayout globalDescriptorSetLayout
	) : sumiDevice{device} {
		
		// Check physical device can support the size of push constants for this pipeline.
		//  VK min guarantee is 128 bytes, this pipeline targets 256 bytes.
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &deviceProperties);
		uint32_t requiredPushConstantSize = static_cast<uint32_t>(
			sizeof(structs::VertPushConstantData) + sizeof(structs::FragPushConstantData
		));
		if (deviceProperties.limits.maxPushConstantsSize < requiredPushConstantSize) {
			throw std::runtime_error(
				"Mesh rendering requires at least" + 
				std::to_string(requiredPushConstantSize) +
				" bytes of push constant storage. Physical device used supports only " +
				std::to_string(deviceProperties.limits.maxPushConstantsSize) +
				" bytes."
			);
		}

		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines(renderPass, subpassIdx);
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

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
			"[Sumire::MeshRenderSys] Failed to create mesh rendering pipeline layout."
		);
	}

	void MeshRenderSys::createPipelines(VkRenderPass renderPass, uint32_t subpassIdx) {
		assert(pipelineLayout != nullptr && "[Sumire::MeshRenderSys]: Cannot create pipeline before pipeline layout.");

		// TODO: This pipeline creation step should be moved to a common location so that it can be
		//		 re-used by different render systems. This would also allow for efficient caching
		//		 of these pipelines (i.e. do not recreate if it already exists.)
		// TODO: For now, we can generate all permutations of required pipelines during initialization
		//		 so long as the number of flags remains small. If/when it gets larger, we should generate the
		//		 required pipelines at runtime, with (runtime) caching, and generate a common (serialized) 
		//		 pipeline cache that can load up frequently used pipelines from disk.

		// Default pipeline config used as base of permutations
		PipelineConfigInfo defaultConfig{};
		SumiPipeline::defaultPipelineConfigInfo(defaultConfig);
		defaultConfig.renderPass = renderPass;
		defaultConfig.subpass = subpassIdx;
		defaultConfig.pipelineLayout = pipelineLayout;
		std::string defaultVertShader = SUMIRE_ENGINE_PATH("shaders/forward/mesh.vert.spv");
		std::string defaultFragShader = SUMIRE_ENGINE_PATH("shaders/forward/mesh.frag.spv");

		// All permutations (including default)
		const uint32_t pipelinePermutations = 2 * SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_HIGHEST;
		for (uint32_t i = 0; i < pipelinePermutations; i++) {
			SumiPipelineStateFlags permutationFlags = static_cast<SumiPipelineStateFlags>(i);
			PipelineConfigInfo permutationConfig = defaultConfig;
			std::string permutationVertShader = defaultVertShader;
			std::string permutationFragShader = defaultFragShader;

			// Deal with each bit flag
			if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_UNLIT_BIT)
				permutationFragShader = SUMIRE_ENGINE_PATH("shaders/forward/mesh_unlit.frag.spv");
			if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_DOUBLE_SIDED_BIT)
				permutationConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

			// Create pipeline and map it
			std::unique_ptr<SumiPipeline> permutationPipeline = std::make_unique<SumiPipeline>(
				sumiDevice,
				permutationVertShader, 
				permutationFragShader,
				permutationConfig
			);
			pipelines.emplace(permutationFlags, std::move(permutationPipeline));
		}
	}

	void MeshRenderSys::renderObjects(VkCommandBuffer commandBuffer, FrameInfo &frameInfo) {

		vkCmdBindDescriptorSets(
			commandBuffer,
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
				commandBuffer, 
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
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer, pipelineLayout, pipelines);
		}
	}
}