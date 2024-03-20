#include <sumire/core/rendering/sumi_renderer.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <stdexcept>
#include <array>

namespace sumire {

	SumiRenderer::SumiRenderer(SumiWindow &window, SumiDevice &device) : sumiWindow{window}, sumiDevice{device} {
		recreateSwapChain();
		recreateGbuffer();
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
                throw std::runtime_error("[Sumire::SumiRenderer] Swap chain image / depth format has changed!");
            }
		}

		scRecreatedFlag = true;
	}

	void SumiRenderer::recreateGbuffer() {
		// TODO: The default behaviour of the gbuffer is to match the window size, but it may be useful
		//        To have a gbuffer which doesn't match the window dimensions (i.e. higher res, etc).
		auto extent = sumiWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			// Wait on minimization
			extent = sumiWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(sumiDevice.device());

		gbuffer = std::make_unique<SumiGbuffer>(sumiDevice, extent.width, extent.height);
	}

	void SumiRenderer::createCommandBuffers() {

		deferredCommandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = sumiDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(deferredCommandBuffers.size());

		// Deferred rendering command buffers
		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &allocInfo, deferredCommandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate deferred command buffers."
		);

		// Swap Chain Command Buffers
		// 1-to-1 relationship on command buffers -> frame buffers.
		commandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &allocInfo, commandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate swap chain command buffers."
		);
	}

	void SumiRenderer::freeCommandBuffers() {
		VkCommandPool commandPool = sumiDevice.getCommandPool();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			commandPool,
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data()
		);

		commandBuffers.clear();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			commandPool,
			static_cast<uint32_t>(deferredCommandBuffers.size()),
			deferredCommandBuffers.data()
		);

		deferredCommandBuffers.clear();
	}

	SumiRenderer::FrameCommandBuffers SumiRenderer::getCurrentCommandBuffers() const {
		assert(isFrameStarted && "Failed to get command buffers - no frame in flight.");

		FrameCommandBuffers frameCommandBuffers{};
		frameCommandBuffers.deferred = deferredCommandBuffers[currentFrameIdx];
		frameCommandBuffers.swapChain = commandBuffers[currentFrameIdx];

		return frameCommandBuffers;
	}

    SumiRenderer::FrameCommandBuffers SumiRenderer::beginFrame() {
        assert(!isFrameStarted 
			&& "Failed to start frame - frame is already in flight.");

		auto result = sumiSwapChain->acquireNextImage(&currentImageIdx);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return FrameCommandBuffers{};
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			// TODO: Handle this error properly - can occur on window resize.
			throw std::runtime_error("[Sumire::SumiRenderer] Failed to acquire swap chain image");
		}
    
        isFrameStarted = true;

		// Begin buffer recording
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		FrameCommandBuffers frameCommandBuffers = getCurrentCommandBuffers();

		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.deferred, &beginInfo),
			"[Sumire::SumiRenderer] Failed to beging recording of deferred command buffer."
		)

		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.swapChain, &beginInfo),
			"[Sumire::SumiRenderer] Failed to beging recording of swapchain command buffer."
		)

		return frameCommandBuffers;
    }

    void SumiRenderer::endFrame() {
        assert(isFrameStarted 
			&& "Failed to end frame - no frame in flight.");

		// End command buffer recording
		FrameCommandBuffers frameCommandBuffers = getCurrentCommandBuffers();

		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.deferred),
			"[Sumire::SumiRenderer] Failed to end recording of deferred command buffer."
		);

		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.swapChain),
			"[Sumire::SumiRenderer] Failed to end recording of swap chain command buffer."
		)

		// Submitted work for the deferred pass needs to be executed first.
		VkSemaphore imgAvailableSemaphore = sumiSwapChain->getCurrentImageAvailableSemaphore();
		VkPipelineStageFlags defferedWaitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		gbuffer->submitCommandBuffer(
			&frameCommandBuffers.deferred,
			1,
			&imgAvailableSemaphore,
			&defferedWaitFlags
		);

		// Subsequent work needs to wait for the deffered work to be finished before it can start execution
		//	 Internally, SumiSwapChain will wait for images to become available, so we just have to pass in
		//	 any *additional* wait dependencies.
		VkSemaphore deferredFinishedSemaphore = gbuffer->getRenderFinishedSemaphore();
		VkPipelineStageFlags additionalSwapchainWaitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        auto result = sumiSwapChain->submitCommandBuffers(
			&frameCommandBuffers.swapChain, 
			&currentImageIdx,
			1,
			&deferredFinishedSemaphore,
			&additionalSwapchainWaitFlags
		);

		// TODO: Need to consider gbuffer resize too.
		if (result == VK_ERROR_OUT_OF_DATE_KHR || 
				result == VK_SUBOPTIMAL_KHR || 
				sumiWindow.wasWindowResized()) {
			sumiWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
            throw std::runtime_error("[Sumire::SumiRenderer] Failed to present swap chain image.");
        }

        isFrameStarted = false;
        currentFrameIdx = (currentFrameIdx + 1) % SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

	void SumiRenderer::beginGbufferRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Failed to begin deferred render pass - no frame in flight.");
		gbuffer->beginRenderPass(commandBuffer);
	}

	void SumiRenderer::endGbufferRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Failed to end deferred render pass - no frame in flight.");
		gbuffer->endRenderPass(commandBuffer);
	}

    void SumiRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted 
			&& "Failed to begin render pass - no frame in flight.");
        assert(commandBuffer == getCurrentCommandBuffers().swapChain
			&& "Failed to start render pass: beginSwapChainRenderPass() was given a command buffer from a different frame.");

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
		viewport.y = static_cast<float>(sumiSwapChain->getSwapChainExtent().height);
		viewport.width = static_cast<float>(sumiSwapChain->getSwapChainExtent().width);
		viewport.height = -static_cast<float>(sumiSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, sumiSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SumiRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted 
			&& "Failed to end render pass - no frame in flight.");
        assert(commandBuffer == getCurrentCommandBuffers().swapChain
			&& "Failed to end render pass: beginSwapChainRenderPass() was given a command buffer from a different frame."
        );

        vkCmdEndRenderPass(commandBuffer);
    }
}