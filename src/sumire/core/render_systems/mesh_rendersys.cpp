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
			
		createPipelineLayout(globalDescriptorSetLayout);
		createPipeline(renderPass);
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

			structs::VertPushConstantData push{};
			push.modelMatrix = obj.transform.mat4();

			vkCmdPushConstants(
				frameInfo.commandBuffer, 
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(structs::VertPushConstantData),
				&push
			);
			
			// SumiModel handles the binding of descriptor sets 1-3 and frag push constants
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer, pipelineLayout);
		}
	}
}