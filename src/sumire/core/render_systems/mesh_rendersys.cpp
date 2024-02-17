#include <sumire/core/render_systems/mesh_rendersys.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	struct ModelPushConstantData {
		glm::mat4 modelMatrix;
	};

	MeshRenderSys::MeshRenderSys(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		) : sumiDevice{device} {
			
		createPipelineLayout(globalDescriptorSetLayout);
		createPipeline(renderPass);
	}

	MeshRenderSys::~MeshRenderSys() {
		// TODO: Destroy pipeline at this line??
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	void MeshRenderSys::createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout) {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(ModelPushConstantData);
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(
				sumiDevice.device(), 
				&pipelineLayoutInfo,
				nullptr, 
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("<MeshRenderSys>: Failed to create pipeline layout.");
		}
	}

	void MeshRenderSys::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "<MeshRenderSys>: Cannot create pipeline before pipeline layout.");

		PipelineConfigInfo pipelineConfig{};
		SumiPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		sumiPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/simple_mesh.vert.spv",
			"shaders/simple_mesh.frag.spv",
			pipelineConfig);
	}

	void MeshRenderSys::renderObjects(FrameInfo &frameInfo) {
		sumiPipeline->bind(frameInfo.commandBuffer);

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

			ModelPushConstantData push{};
			auto modelMatrix = obj.transform.mat4();
			push.modelMatrix = modelMatrix;

			vkCmdPushConstants(
				frameInfo.commandBuffer, 
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(ModelPushConstantData),
				&push
			);
			
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
	}
}