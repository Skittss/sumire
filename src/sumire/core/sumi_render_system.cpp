#include <sumire/core/sumi_render_system.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	struct SimplePushConstantData {
		glm::mat4 transform;
        alignas(16) glm::vec3 colour; // Deal with device reading push constants not tightly packed
	};

	SumiRenderSystem::SumiRenderSystem(SumiDevice& device, VkRenderPass renderPass) : sumiDevice{device} {
		createPipelineLayout();
		createPipeline(renderPass);
	}

	SumiRenderSystem::~SumiRenderSystem() {
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	void SumiRenderSystem::createPipelineLayout() {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(
				sumiDevice.device(), 
				&pipelineLayoutInfo,
				nullptr, 
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout");
		}
	}

	void SumiRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		SumiPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		sumiPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig);
	}

	void SumiRenderSystem::renderObjects(VkCommandBuffer commandBuffer, std::vector<SumiObject> &objects, const SumiCamera& camera) {
		sumiPipeline->bind(commandBuffer);

		auto cameraMatrix = camera.getProjectionMatrix() * camera.getViewMatrix();

		for (auto& obj: objects) {

			SimplePushConstantData push{};
			push.colour = obj.colour;
			push.transform = cameraMatrix * obj.transform.mat4();

			vkCmdPushConstants(
				commandBuffer, 
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}
}