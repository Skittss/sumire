#include "tile_deferred_renderer.hpp"

#include <stdexcept>
#include <array>
#include <cassert>

namespace sde {

	TileDeferredRenderer::TileDeferredRenderer() {
		loadModels();
		createPipelineLayout();
		recreateSwapChain();
		createCommandBuffers();
	}

	TileDeferredRenderer::~TileDeferredRenderer() {
		vkDestroyPipelineLayout(sdeDevice.device(), pipelineLayout, nullptr);
	}

	void TileDeferredRenderer::run() {
		while (!sdeWindow.shouldClose()) {
			glfwPollEvents();
			drawFrame();
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		vkDeviceWaitIdle(sdeDevice.device());
	}

	void TileDeferredRenderer::loadModels() {
		std::vector<SdeModel::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

		sdeModel = std::make_unique<SdeModel>(sdeDevice, vertices);
	}

	void TileDeferredRenderer::createPipelineLayout() {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(
				sdeDevice.device(), 
				&pipelineLayoutInfo,
				nullptr, 
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout");
		}
	}

	void TileDeferredRenderer::createPipeline() {
		assert(sdeSwapChain != nullptr && "cannot create pipeline before swap chain");
		assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		SdePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = sdeSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		sdePipeline = std::make_unique<SdePipeline>(
			sdeDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig);
	}

	void TileDeferredRenderer::recreateSwapChain() {
		auto extent = sdeWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			// Wait on minimization
			extent = sdeWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(sdeDevice.device());
		
		if (sdeSwapChain == nullptr) {
			sdeSwapChain = std::make_unique<SdeSwapChain>(sdeDevice, extent);
		}
		else {
			sdeSwapChain = std::make_unique<SdeSwapChain>(sdeDevice, extent, std::move(sdeSwapChain));
			if (sdeSwapChain->imageCount() != commandBuffers.size()) {
				freeCommandBuffers();
				createCommandBuffers();
			}
		}
		// TODO: If new render pass is compatible, we needn't create a new pipeline.
		createPipeline();
	}

	void TileDeferredRenderer::createCommandBuffers() {
		// 1-to-1 relationship on command buffers -> frame buffers.
		commandBuffers.resize(sdeSwapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		// We allocate the memory for command buffers once beforehand in a *pool* to save on
		//  the cost of creating a buffer at runtime, as this is a frequent operation.
		allocInfo.commandPool = sdeDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(sdeDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void TileDeferredRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(
			sdeDevice.device(),
			sdeDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());

		commandBuffers.clear();
	}

	void TileDeferredRenderer::recordCommandBuffer(int imageIdx) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIdx], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = sdeSwapChain->getRenderPass();
		renderPassInfo.framebuffer = sdeSwapChain->getFrameBuffer(imageIdx);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = sdeSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[imageIdx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(sdeSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(sdeSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, sdeSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIdx], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIdx], 0, 1, &scissor);

		sdePipeline->bind(commandBuffers[imageIdx]);
		sdeModel->bind(commandBuffers[imageIdx]);
		sdeModel->draw(commandBuffers[imageIdx]);

		vkCmdEndRenderPass(commandBuffers[imageIdx]);

		if (vkEndCommandBuffer(commandBuffers[imageIdx]) != VK_SUCCESS) {
			throw std::runtime_error("failed to end recording of command buffer");
		}
	}

	void TileDeferredRenderer::drawFrame() {
		uint32_t imageIndex;
		auto result = sdeSwapChain->acquireNextImage(&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			// TODO: Handle this error properly - can occur on window resize.
			throw std::runtime_error("failed to acquire swap chain image");
		}
	
		recordCommandBuffer(imageIndex);
		result = sdeSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || 
				result == VK_SUBOPTIMAL_KHR || 
				sdeWindow.wasWindowResized()) {
			sdeWindow.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
	}

}