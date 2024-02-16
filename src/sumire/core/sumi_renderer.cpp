#include <sumire/core/sumi_renderer.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	SumiRenderer::SumiRenderer(SumiWindow &window, SumiDevice &device) : sumiWindow{window}, sumiDevice{device} {
		recreateSwapChain();
		createCommandBuffers();
	}

	SumiRenderer::~SumiRenderer() {
		freeCommandBuffers();
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
            std::shared_ptr<SumiSwapChain> oldSwapChain = std::move(sumiSwapChain);
			sumiSwapChain = std::make_unique<SumiSwapChain>(sumiDevice, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*sumiSwapChain.get())) {
                throw std::runtime_error("Swap chain image / depth format has changed!");
            }
		}

		scRecreatedFlag = true;

		// TODO: If new render pass is compatible, we needn't create a new pipeline.
		//createPipeline();
	}

	void SumiRenderer::createCommandBuffers() {
		// 1-to-1 relationship on command buffers -> frame buffers.
		commandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

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

    VkCommandBuffer SumiRenderer::beginFrame(){
        assert(!isFrameStarted && "Failed to start frame (frame is already in flight).");

		auto result = sumiSwapChain->acquireNextImage(&currentImageIdx);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			// TODO: Handle this error properly - can occur on window resize.
			throw std::runtime_error("Failed to acquire swap chain image");
		}
    
        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();
            VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

        return commandBuffer;
    }

    void SumiRenderer::endFrame() {
        assert(isFrameStarted && "Failed to end frame (frame is not in flight).");

        auto commandBuffer = getCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to end recording of command buffer.");
		}

        auto result = sumiSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIdx);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || 
				result == VK_SUBOPTIMAL_KHR || 
				sumiWindow.wasWindowResized()) {
			sumiWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        isFrameStarted = false;
        currentFrameIdx = (currentFrameIdx + 1) % SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void SumiRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Failed to begin render pass (frame is not in flight).");
        assert(
            commandBuffer == getCurrentCommandBuffer() && 
            "Failed to start render pass: beginSwapChainRenderPass() was given a command buffer from a different frame."
        );

        VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = sumiSwapChain->getRenderPass();
		renderPassInfo.framebuffer = sumiSwapChain->getFrameBuffer(currentImageIdx);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = sumiSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(sumiSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(sumiSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, sumiSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SumiRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Failed to end render pass (frame is not in flight).");
        assert(
            commandBuffer == getCurrentCommandBuffer() && 
            "Failed to end render pass: beginSwapChainRenderPass() was given a command buffer from a different frame."
        );

        vkCmdEndRenderPass(commandBuffer);
    }
}