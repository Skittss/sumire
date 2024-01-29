#include "sumirenderer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	struct SimplePushConstantData {
		alignas(16) glm::vec3 colour; // Deal with device reading push constants not tightly packed
		glm::mat4 transform;
	};

	SumiRenderer::SumiRenderer() {
		loadObjects();
		createPipelineLayout();
		recreateSwapChain();
		createCommandBuffers();
	}

	SumiRenderer::~SumiRenderer() {
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	void SumiRenderer::run() {
		while (!sumiWindow.shouldClose()) {
			glfwPollEvents();
			drawFrame();
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		vkDeviceWaitIdle(sumiDevice.device());
	}

	void SumiRenderer::loadObjects() {
		std::vector<SumiModel::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

		auto model = std::make_shared<SumiModel>(sumiDevice, vertices);

		auto triangle = SumiObject::createObject();
		triangle.model = model;
		triangle.colour = {1.0f, 0.0f, 0.0f};
		triangle.transform = glm::mat4();

		objects.push_back(std::move(triangle));

	}

	void SumiRenderer::createPipelineLayout() {

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

	void SumiRenderer::createPipeline() {
		assert(sumiSwapChain != nullptr && "cannot create pipeline before swap chain");
		assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		SumiPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = sumiSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		sumiPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig);
	}

	void SumiRenderer::recreateSwapChain() {
		auto extent = sumiWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			// Wait on minimization
			extent = sumiWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(sumiDevice.device());
		
		if (sumiSwapChain == nullptr) {
			sumiSwapChain = std::make_unique<SumiSwapChain>(sumiDevice, extent);
		}
		else {
			sumiSwapChain = std::make_unique<SumiSwapChain>(sumiDevice, extent, std::move(sumiSwapChain));
			if (sumiSwapChain->imageCount() != commandBuffers.size()) {
				// make new cb
				freeCommandBuffers();
				createCommandBuffers();
			}
		}
		// TODO: If new render pass is compatible, we needn't create a new pipeline.
		createPipeline();
	}

	void SumiRenderer::createCommandBuffers() {
		// 1-to-1 relationship on command buffers -> frame buffers.
		commandBuffers.resize(sumiSwapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		// We allocate the memory for command buffers once beforehand in a *pool* to save on
		//  the cost of creating a buffer at runtime, as this is a frequent operation.
		allocInfo.commandPool = sumiDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(sumiDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void SumiRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(
			sumiDevice.device(),
			sumiDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());

		commandBuffers.clear();
	}

	void SumiRenderer::recordCommandBuffer(int imageIdx) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIdx], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = sumiSwapChain->getRenderPass();
		renderPassInfo.framebuffer = sumiSwapChain->getFrameBuffer(imageIdx);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = sumiSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[imageIdx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(sumiSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(sumiSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, sumiSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIdx], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIdx], 0, 1, &scissor);

		renderObjects(commandBuffers[imageIdx]);
		
		vkCmdEndRenderPass(commandBuffers[imageIdx]);

		if (vkEndCommandBuffer(commandBuffers[imageIdx]) != VK_SUCCESS) {
			throw std::runtime_error("failed to end recording of command buffer");
		}
	}

	void SumiRenderer::renderObjects(VkCommandBuffer commandBuffer) {
		sumiPipeline->bind(commandBuffer);

		for (auto& obj: objects) {
			SimplePushConstantData push{};
			push.colour = obj.colour;
			push.transform = obj.transform;

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

	void SumiRenderer::drawFrame() {
		uint32_t imageIndex;
		auto result = sumiSwapChain->acquireNextImage(&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			// TODO: Handle this error properly - can occur on window resize.
			throw std::runtime_error("failed to acquire swap chain image");
		}
	
		recordCommandBuffer(imageIndex);
		result = sumiSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || 
				result == VK_SUBOPTIMAL_KHR || 
				sumiWindow.wasWindowResized()) {
			sumiWindow.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
	}

}