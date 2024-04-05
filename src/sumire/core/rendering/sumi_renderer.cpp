#include <sumire/core/rendering/sumi_renderer.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <stdexcept>
#include <array>

namespace sumire {

	SumiRenderer::SumiRenderer(SumiWindow &window, SumiDevice &device) : sumiWindow{window}, sumiDevice{device} {
		recreateSwapChain();
		recreateGbuffer();
		createRenderPass();
		createFramebuffers();
		createCommandBuffers();
	}

	SumiRenderer::~SumiRenderer() {
		freeCommandBuffers();
		freeFramebuffers();

		vkDestroyRenderPass(sumiDevice.device(), renderPass, nullptr);
	}

	void SumiRenderer::recreateRenderObjects() {
		recreateSwapChain();
		recreateGbuffer();

		freeFramebuffers();
		createFramebuffers();
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
                throw std::runtime_error("[Sumire::SumiRenderer] Could not use old swapchain as base for recreation - swap chain format has changed.");
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

		VkImageUsageFlags gbufferAttachmentExtraFlags = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		gbuffer = std::make_unique<SumiGbuffer>(
			sumiDevice, 
			extent,
			gbufferAttachmentExtraFlags
		);

		gbufferRecreatedFlag = true;
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
	
	void SumiRenderer::createRenderPass() {
		// Create the renderpass used to render a frame.
		// This currently involves 3 subpasses:
		//    - Gbuffer fill
		//    - Gbuffer resolve
		//    - Forward render to swapchain (for UI, etc.)

		// Attachment descriptions
		std::array<VkAttachmentDescription, 6> attachmentDescriptions{};

		// 0: Swap Chain Color
		attachmentDescriptions[0].format = sumiSwapChain->getColorFormat();
		attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// 1: Gbuffer Position
		attachmentDescriptions[1].format = gbuffer->positionAttachment()->getFormat();
		attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 2: Gbuffer Normal
		attachmentDescriptions[2].format = gbuffer->normalAttachment()->getFormat();
		attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 3: Gbuffer Albedo
		attachmentDescriptions[3].format = gbuffer->albedoAttachment()->getFormat();
		attachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 4: Gbuffer AoMetalRoughEmissive (PBR)
		attachmentDescriptions[4].format = gbuffer->aoMetalRoughEmissiveAttachment()->getFormat();
		attachmentDescriptions[4].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 5: Shared Depth
		attachmentDescriptions[5].format = sumiSwapChain->findDepthFormat();
		attachmentDescriptions[5].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[5].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[5].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[5].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[5].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::array<VkSubpassDescription, 3> subpassDescriptions{};
		std::array<VkSubpassDependency, 5> subpassDependencies{};

		// 0 - Gbuffer Fill ------------------------------------------------------------------
		// Use 5 color attachments (gbuffer + swap chain image) 
		//   so that unlit meshes can be rendered straight to the swap chain, bypassing the lighting resolve.
		std::array<VkAttachmentReference, 5> gbufferFillColorRefs{};
		gbufferFillColorRefs[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		gbufferFillColorRefs[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		gbufferFillColorRefs[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		gbufferFillColorRefs[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		gbufferFillColorRefs[4] = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference gbufferFillDepthRef = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount = static_cast<uint32_t>(gbufferFillColorRefs.size());
		subpassDescriptions[0].pColorAttachments = gbufferFillColorRefs.data();
		subpassDescriptions[0].pDepthStencilAttachment = &gbufferFillDepthRef;

		// --- Dependencies
		// ----- EXT -> 0 - Make sure all previous renderpass depth/color writes are finished before we begin here.
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].srcAccessMask = 0x0;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0x0;

		subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].dstSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = 0;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dependencyFlags = 0x0;
		
		// 1 - Gbuffer Resolve  --------------------------------------------------------------
		VkAttachmentReference gbufferResolveColorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		std::array<VkAttachmentReference, 4> gbufferResolveInputReferences{};
		gbufferResolveInputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		gbufferResolveInputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		gbufferResolveInputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		gbufferResolveInputReferences[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		VkAttachmentReference gbufferResolveDepthRef = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[1].colorAttachmentCount = 1;
		subpassDescriptions[1].pColorAttachments = &gbufferResolveColorRef;
		subpassDescriptions[1].pDepthStencilAttachment = &gbufferResolveDepthRef;
		subpassDescriptions[1].inputAttachmentCount = static_cast<uint32_t>(gbufferResolveInputReferences.size());
		subpassDescriptions[1].pInputAttachments = gbufferResolveInputReferences.data();

		// --- Dependencies
		// ----- 0 -> 1 - Transition attachments to input read values.
		subpassDependencies[2].srcSubpass = 0;
		subpassDependencies[2].dstSubpass = 1;
		subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		subpassDependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// 2 - Forward Render To Swapchain ---------------------------------------------------
		// Reuse the depth buffer from the Gbuffer Fill subpass
		VkAttachmentReference forwardColorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference forwardDepthRef = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[2].colorAttachmentCount = 1;
		subpassDescriptions[2].pColorAttachments = &forwardColorRef;
		subpassDescriptions[2].pDepthStencilAttachment = &gbufferResolveDepthRef;
		//subpassDescriptions[2].inputAttachmentCount = static_cast<uint32_t>(forwardInputRefs.size());
		//subpassDescriptions[2].pInputAttachments = forwardInputRefs.data();

		subpassDependencies[3].srcSubpass = 1;
		subpassDependencies[3].dstSubpass = 2;
		subpassDependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		subpassDependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[4].srcSubpass = 2;
		subpassDependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		subpassDependencies[4].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderPassInfo.pSubpasses = subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies = subpassDependencies.data();

		VK_CHECK_SUCCESS(
			vkCreateRenderPass(sumiDevice.device(), &renderPassInfo, nullptr, &renderPass),
			"[Sumire::SumiRenderer] Failed to create render pass."
		);
	}

	void SumiRenderer::createFramebuffers() {

		std::array<VkImageView, 6> attachments{};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = sumiSwapChain->width();
        framebufferInfo.height = sumiSwapChain->height();
        framebufferInfo.layers = 1;

		// Note: We may end up creating more framebuffers than there are 
		//       frames *in flight* due to how the swapchain acquires images.
		framebuffers = std::vector<VkFramebuffer>(sumiSwapChain->imageCount());
		for (uint32_t i = 0; i < framebuffers.size(); i++) {
			
			attachments[0] = sumiSwapChain->getColorAttachment(i)->getImageView();
			attachments[1] = gbuffer->positionAttachment()->getImageView();
			attachments[2] = gbuffer->normalAttachment()->getImageView();
			attachments[3] = gbuffer->albedoAttachment()->getImageView();
			attachments[4] = gbuffer->aoMetalRoughEmissiveAttachment()->getImageView();
			attachments[5] = sumiSwapChain->getDepthAttachment()->getImageView();

			VK_CHECK_SUCCESS(
				vkCreateFramebuffer(sumiDevice.device(), &framebufferInfo, nullptr, &framebuffers[i]),
				"[Sumire::SumiRenderer] Failed to create framebuffer (idx=" + std::to_string(i) + ") for main render pass."
			);
		}
	}

	void SumiRenderer::freeFramebuffers() {
		for (auto fb : framebuffers) {
			vkDestroyFramebuffer(sumiDevice.device(), fb, nullptr);
		}
	}

	SumiRenderer::FrameCommandBuffers SumiRenderer::getCurrentCommandBuffers() const {
		assert(isFrameStarted && "Failed to get command buffers - no frame in flight.");

		FrameCommandBuffers frameCommandBuffers{};
		frameCommandBuffers.swapChain = commandBuffers[currentFrameIdx];

		return frameCommandBuffers;
	}

    SumiRenderer::FrameCommandBuffers SumiRenderer::beginFrame() {
        assert(!isFrameStarted 
			&& "Failed to start frame - frame is already in flight.");

		auto result = sumiSwapChain->acquireNextImage(currentFrameIdx, &currentImageIdx);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateRenderObjects();
			//recreateSwapChain();
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
			vkEndCommandBuffer(frameCommandBuffers.swapChain),
			"[Sumire::SumiRenderer] Failed to end recording of swap chain command buffer."
		)

		VkSubmitInfo submitInfo{};
		VkSemaphore frameAvailable = sumiSwapChain->getImageAvailableSemaphore(currentFrameIdx);
		VkSemaphore renderFinished = sumiSwapChain->getRenderFinishedSemaphore(currentFrameIdx);
		VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frameCommandBuffers.swapChain;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frameAvailable;
		submitInfo.pWaitDstStageMask = &waitStageFlag;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinished;

		// If this frame is in flight, we must wait for it to finish before submitting more work,
		//  else we will end up with a work backlog / compute multiple batches once the semaphore signals
		
		// For the first few frames, no fences will have been signaled, so we should only wait
		//   if the fence exists.
		VkFence frameInFlightFence = sumiSwapChain->getImageInFlightFence(currentImageIdx);
		if (frameInFlightFence != VK_NULL_HANDLE) {
			vkWaitForFences(sumiDevice.device(), 1, &frameInFlightFence, VK_TRUE, UINT64_MAX);
		}
		sumiSwapChain->setImageInFlightFence(currentImageIdx, currentFrameIdx);
		// reaccess the frame to guarantee a non-null handle
		frameInFlightFence = sumiSwapChain->getImageInFlightFence(currentImageIdx);
		
		// Reset fence pending queue submission.
		vkResetFences(sumiDevice.device(), 1, &frameInFlightFence);
		VK_CHECK_SUCCESS(
			vkQueueSubmit(sumiDevice.graphicsQueue(), 1, &submitInfo, frameInFlightFence),
			"[Sumire::SumiRenderer] Could not submit frame command buffer."
		);

		auto result = sumiSwapChain->queuePresent(&currentImageIdx, 1, &renderFinished);

		// TODO: Need to consider gbuffer resize too.
		if (
			result == VK_ERROR_OUT_OF_DATE_KHR ||
			result == VK_SUBOPTIMAL_KHR || 
			sumiWindow.wasWindowResized()
		) {
			sumiWindow.resetWindowResizedFlag();
			recreateRenderObjects();
			//recreateSwapChain();
		} else if (result != VK_SUCCESS) {
            throw std::runtime_error("[Sumire::SumiRenderer] Failed to present swap chain image.");
        }

        isFrameStarted = false;
        currentFrameIdx = (currentFrameIdx + 1) % SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

	void SumiRenderer::beginRenderPass(VkCommandBuffer commandBuffer) {
		currentSubpass = 0;

		// Begin renderpass for gbuffer fill
        std::array<VkClearValue, 6> attachmentClearValues{};
        attachmentClearValues[0].color = { { 0.01f, 0.01f, 0.01f, 1.0f } };
        attachmentClearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[4].color = { { 1.0f, 0.0f, 0.0f, 0.0f } }; // AO, Metallic, Roughness, Emissive
        attachmentClearValues[5].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[currentImageIdx];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = sumiSwapChain->getExtent();
        renderPassInfo.clearValueCount = static_cast<uint32_t>(attachmentClearValues.size());
        renderPassInfo.pClearValues = attachmentClearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(sumiSwapChain->height());
        viewport.width = static_cast<float>(sumiSwapChain->width());
        viewport.height = -static_cast<float>(sumiSwapChain->height());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{ {0, 0}, sumiSwapChain->getExtent() };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void SumiRenderer::nextSubpass(VkCommandBuffer commandBuffer) {
		currentSubpass++;

		if (currentSubpass > 3) {
			throw std::runtime_error(
				"[Sumire::SumiRenderer] Could not advance to next subpass - subpass index out of range.");
		}

		// TODO: Potentially change renderpass info 

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	}

	void SumiRenderer::endRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Failed to end render pass - no frame in flight.");
        assert(commandBuffer == getCurrentCommandBuffers().swapChain
			&& "Failed to end render pass: Command buffer provided did not record this renderpass."
        );

        vkCmdEndRenderPass(commandBuffer);
	}
}