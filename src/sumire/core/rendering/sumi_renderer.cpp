#include <sumire/core/rendering/sumi_renderer.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <stdexcept>
#include <array>

namespace sumire {

	SumiRenderer::SumiRenderer(
		SumiWindow &window, SumiDevice &device, SumiConfig &config
	) : sumiWindow{ window }, sumiDevice{ device }, sumiConfig{ config } {
		swapChainUseVsync = config.configData.VSYNC;

		recreateSwapChain();
		recreateGbuffer();
		createRenderPass();
		createCompositionRenderPass();
		createFramebuffers();
		createCommandBuffers();
		createSyncObjects();
	}

	SumiRenderer::~SumiRenderer() {
		freeSyncObjects();
		freeCommandBuffers();
		freeFramebuffers();

		vkDestroyRenderPass(sumiDevice.device(), renderPass, nullptr);
		vkDestroyRenderPass(sumiDevice.device(), compositionRenderPass, nullptr);
	}

	void SumiRenderer::recreateRenderObjects() {
		recreateSwapChain();
		recreateGbuffer();

		freeFramebuffers();
		createFramebuffers();

		if (SumiSwapChain::MAX_FRAMES_IN_FLIGHT != graphicsFinishedSemaphores.size()) {
			freeSyncObjects();
			createSyncObjects();
		}
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
			sumiSwapChain = std::make_unique<SumiSwapChain>(sumiDevice, extent, swapChainUseVsync);
		}
		else {
            std::shared_ptr<SumiSwapChain> oldSwapChain = std::move(sumiSwapChain);
			sumiSwapChain = std::make_unique<SumiSwapChain>(sumiDevice, extent, oldSwapChain, swapChainUseVsync);

            if (!oldSwapChain->compareSwapFormats(*sumiSwapChain.get())) {
                throw std::runtime_error("[Sumire::SumiRenderer] Could not use old swapchain as base for recreation - swap chain format has changed.");
            }
		}

		// Recreate mirror attachments for intermedaite rendering
		intermediateColorAttachments.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkFormat colFormat = getIntermediateColorAttachmentFormat();

		// Check supported format properties correspond with the ones we need
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(sumiDevice.getPhysicalDevice(), colFormat, &formatProperties);
		assert(
			formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT &&
			formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
			formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
			&& "Image format used for swapchain mirror does not support required operations (color attachment, sampled, storage)."
		);

		for (uint32_t i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			intermediateColorAttachments[i] = std::make_unique<SumiAttachment>(
				sumiDevice,
				extent,
				colFormat,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			);
		}

		scRecreatedFlag = true;
		swapChainNeedsRecreate = false;
	}
	
	// Change the swap chain's present mode. Queue's recreation for the start of the next frame.
	void SumiRenderer::changeSwapChainPresentMode(bool vsync) {
		swapChainUseVsync = vsync;
		swapChainNeedsRecreate = true;
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
		// 1-to-1 relationship on command buffers -> frame buffers.

		// Pre-draw compute command buffers
		predrawComputeCommandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo predrawComputeAllocInfo{};
		predrawComputeAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		predrawComputeAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		predrawComputeAllocInfo.commandPool = sumiDevice.getComputeCommandPool();
		predrawComputeAllocInfo.commandBufferCount = static_cast<uint32_t>(predrawComputeCommandBuffers.size());

		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &predrawComputeAllocInfo, predrawComputeCommandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate post command buffers."
		);

		// Graphics Command Buffers
		graphicsCommandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo graphicsAllocInfo{};
		graphicsAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		graphicsAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		graphicsAllocInfo.commandPool = sumiDevice.getGraphicsCommandPool();
		graphicsAllocInfo.commandBufferCount = static_cast<uint32_t>(graphicsCommandBuffers.size());

		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &graphicsAllocInfo, graphicsCommandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate graphics command buffers."
		);

		// Present Command Buffers
		presentCommandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo presentAllocInfo{};
		presentAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		presentAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		presentAllocInfo.commandPool = sumiDevice.getPresentCommandPool();
		presentAllocInfo.commandBufferCount = static_cast<uint32_t>(presentCommandBuffers.size());

		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &presentAllocInfo, presentCommandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate present command buffers."
		);

		// Post compute command buffers
		computeCommandBuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo computeAllocInfo{};
		computeAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		computeAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		computeAllocInfo.commandPool = sumiDevice.getComputeCommandPool();
		computeAllocInfo.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

		VK_CHECK_SUCCESS(
			vkAllocateCommandBuffers(sumiDevice.device(), &computeAllocInfo, computeCommandBuffers.data()),
			"[Sumire::SumiRenderer] Failed to allocate post command buffers."
		);
	}

	void SumiRenderer::freeCommandBuffers() {
		VkCommandPool predrawComputeCommandPool = sumiDevice.getComputeCommandPool();
		VkCommandPool graphicsCommandPool = sumiDevice.getGraphicsCommandPool();
		VkCommandPool presentCommandPool = sumiDevice.getPresentCommandPool();
		VkCommandPool computeCommandPool = sumiDevice.getComputeCommandPool();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			predrawComputeCommandPool,
			static_cast<uint32_t>(predrawComputeCommandBuffers.size()),
			predrawComputeCommandBuffers.data()
		);
		predrawComputeCommandBuffers.clear();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			graphicsCommandPool,
			static_cast<uint32_t>(graphicsCommandBuffers.size()),
			graphicsCommandBuffers.data()
		);
		graphicsCommandBuffers.clear();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			presentCommandPool,
			static_cast<uint32_t>(presentCommandBuffers.size()),
			presentCommandBuffers.data()
		);
		presentCommandBuffers.clear();

		vkFreeCommandBuffers(
			sumiDevice.device(),
			computeCommandPool,
			static_cast<uint32_t>(computeCommandBuffers.size()),
			computeCommandBuffers.data()
		);
		computeCommandBuffers.clear();
	}

	void SumiRenderer::createSyncObjects() {
		predrawComputeFinishedSemaphores.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		graphicsFinishedSemaphores.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		postComputeFinishedSemaphores.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			VK_CHECK_SUCCESS(
				vkCreateSemaphore(sumiDevice.device(), &semaphoreInfo, nullptr, &predrawComputeFinishedSemaphores[i]),
				"[Sumire::SumiRenderer] Failed to create graphics signaling semaphores."
			);
			VK_CHECK_SUCCESS(
				vkCreateSemaphore(sumiDevice.device(), &semaphoreInfo, nullptr, &graphicsFinishedSemaphores[i]),
				"[Sumire::SumiRenderer] Failed to create graphics signaling semaphores."
			);
			VK_CHECK_SUCCESS(
				vkCreateSemaphore(sumiDevice.device(), &semaphoreInfo, nullptr, &postComputeFinishedSemaphores[i]),
				"[Sumire::SumiRenderer] Failed to create compute signaling semaphores."
			);
		}

	}

	void SumiRenderer::freeSyncObjects() {
		for (auto& semaphore : predrawComputeFinishedSemaphores) {
			vkDestroySemaphore(sumiDevice.device(), semaphore, nullptr);
		}
		for (auto& semaphore : graphicsFinishedSemaphores) {
			vkDestroySemaphore(sumiDevice.device(), semaphore, nullptr);
		}
		for (auto& semaphore : postComputeFinishedSemaphores) {
			vkDestroySemaphore(sumiDevice.device(), semaphore, nullptr);
		}
	}

	std::vector<SumiAttachment*> SumiRenderer::getIntermediateColorAttachments() const {
		std::vector<SumiAttachment*> attachments(intermediateColorAttachments.size());
		for (uint32_t i = 0; i < attachments.size(); i++) {
			attachments[i] = intermediateColorAttachments[i].get();
		}

		return attachments;
	}
	
	VkFormat SumiRenderer::getIntermediateColorAttachmentFormat() const {
		return VK_FORMAT_R16G16B16A16_UNORM;
	}
	
	void SumiRenderer::createRenderPass() {
		// Create the renderpass used to render a frame.
		// This currently involves 3 subpasses:
		//    - Gbuffer fill
		//    - Gbuffer resolve
		//    - Forward render to swapchain (for transparency, etc.)

		// Attachment descriptions
		std::array<VkAttachmentDescription, 6> attachmentDescriptions{};

		// 0: Swap Chain Color (Intermediate targets)
		attachmentDescriptions[0].format = getIntermediateColorAttachmentFormat();
		attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL; //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
			"[Sumire::SumiRenderer] Failed to create scene render pass."
		);
	}

	void SumiRenderer::createCompositionRenderPass() {
		// Attachment descriptions
		std::array<VkAttachmentDescription, 1> attachmentDescriptions{};

		// 0: Swap Chain Color
		attachmentDescriptions[0].format = sumiSwapChain->getColorFormat();
		attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		std::array<VkSubpassDescription, 1> subpassDescriptions{};
		std::array<VkSubpassDependency, 1> subpassDependencies{};

		// 0 - HDR and tonemap ------------------------------------------------------------------
		std::array<VkAttachmentReference, 1> hdrColorRefs{};
		hdrColorRefs[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount = static_cast<uint32_t>(hdrColorRefs.size());
		subpassDescriptions[0].pColorAttachments = hdrColorRefs.data();
		subpassDescriptions[0].pDepthStencilAttachment = nullptr;

		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[0].srcAccessMask = 0x0;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0x0;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderPassInfo.pSubpasses = subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies = subpassDependencies.data();

		VK_CHECK_SUCCESS(
			vkCreateRenderPass(sumiDevice.device(), &renderPassInfo, nullptr, &compositionRenderPass),
			"[Sumire::SumiRenderer] Failed to create post render pass."
		);
	}

	void SumiRenderer::createFramebuffers() {

		// Create Swapchain Fbs
		VkFramebufferCreateInfo swapchainFramebufferInfo{};
		swapchainFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		swapchainFramebufferInfo.renderPass = compositionRenderPass;
		swapchainFramebufferInfo.attachmentCount = 1;
		swapchainFramebufferInfo.pAttachments = nullptr;
		swapchainFramebufferInfo.width = sumiSwapChain->width();
		swapchainFramebufferInfo.height = sumiSwapChain->height();
		swapchainFramebufferInfo.layers = 1;

		swapchainFramebuffers.resize(sumiSwapChain->imageCount());
		for (uint32_t i = 0; i < sumiSwapChain->imageCount(); i++) {
			const VkImageView swapchainImageView = sumiSwapChain->getColorAttachment(i)->getImageView();
			swapchainFramebufferInfo.pAttachments = &swapchainImageView;

			VK_CHECK_SUCCESS(
				vkCreateFramebuffer(sumiDevice.device(), &swapchainFramebufferInfo, nullptr, &swapchainFramebuffers[i]),
				"[Sumire::SumiRenderer] Failed to create swapchain framebuffer (idx=" + std::to_string(i) + ") for main render pass."
			);
		}

		// Create frame pipeline Fbs
		std::array<VkImageView, 6> attachments{};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = sumiSwapChain->width();
		framebufferInfo.height = sumiSwapChain->height();
		framebufferInfo.layers = 1;

		framebuffers.resize(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			
			attachments[0] = intermediateColorAttachments[i]->getImageView();
			attachments[1] = gbuffer->positionAttachment()->getImageView();
			attachments[2] = gbuffer->normalAttachment()->getImageView();
			attachments[3] = gbuffer->albedoAttachment()->getImageView();
			attachments[4] = gbuffer->aoMetalRoughEmissiveAttachment()->getImageView();
			attachments[5] = sumiSwapChain->getDepthAttachment()->getImageView();

			VK_CHECK_SUCCESS(
				vkCreateFramebuffer(sumiDevice.device(), &framebufferInfo, nullptr, &framebuffers[i]),
				"[Sumire::SumiRenderer] Failed to create frame pipeline framebuffers (idx=" + std::to_string(i) + ") for main render pass."
			);
		}
	}

	void SumiRenderer::freeFramebuffers() {
		for (auto fb : framebuffers) {
			vkDestroyFramebuffer(sumiDevice.device(), fb, nullptr);
		}
		for (auto fb : swapchainFramebuffers) {
			vkDestroyFramebuffer(sumiDevice.device(), fb, nullptr);
		}
	}

	SumiRenderer::FrameCommandBuffers SumiRenderer::getCurrentCommandBuffers() const {
		assert(isFrameStarted && "Failed to get command buffers - no frame in flight.");

		FrameCommandBuffers frameCommandBuffers{};
		frameCommandBuffers.predrawCompute = predrawComputeCommandBuffers[currentFrameIdx];
		frameCommandBuffers.graphics = graphicsCommandBuffers[currentFrameIdx];
		frameCommandBuffers.compute = computeCommandBuffers[currentFrameIdx];
		frameCommandBuffers.present = presentCommandBuffers[currentFrameIdx];

		return frameCommandBuffers;
	}

    SumiRenderer::FrameCommandBuffers SumiRenderer::beginFrame() {
        assert(!isFrameStarted 
			&& "Failed to start frame - frame is already in flight.");

		auto result = sumiSwapChain->acquireNextImage(currentFrameIdx, &currentImageIdx);

		if (
			swapChainNeedsRecreate || 
			result == VK_ERROR_OUT_OF_DATE_KHR
		) {
			recreateRenderObjects();
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
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // TODO: Needs test

		FrameCommandBuffers frameCommandBuffers = getCurrentCommandBuffers();

		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.predrawCompute, &beginInfo),
			"[Sumire::SumiRenderer] Failed to begin recording of pre-draw compute command buffer."
		);
		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.graphics, &beginInfo),
			"[Sumire::SumiRenderer] Failed to begin recording of graphics command buffer."
		);
		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.compute, &beginInfo),
			"[Sumire::SumiRenderer] Failed to begin recording of compute command buffer."
		);
		VK_CHECK_SUCCESS(
			vkBeginCommandBuffer(frameCommandBuffers.present, &beginInfo),
			"[Sumire::SumiRenderer] Failed to begin recording of present command buffer."
		);

		return frameCommandBuffers;
    }

    void SumiRenderer::endFrame() {
        assert(isFrameStarted 
			&& "Failed to end frame - no frame in flight.");

		// End command buffer recording
		FrameCommandBuffers frameCommandBuffers = getCurrentCommandBuffers();

		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.predrawCompute),
			"[Sumire::SumiRenderer] Failed to end recording of pre-draw compute command buffer."
		);
		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.graphics),
			"[Sumire::SumiRenderer] Failed to end recording of graphics command buffer."
		);
		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.compute),
			"[Sumire::SumiRenderer] Failed to end recording of compute command buffer."
		);
		VK_CHECK_SUCCESS(
			vkEndCommandBuffer(frameCommandBuffers.present),
			"[Sumire::SumiRenderer] Failed to end recording of present command buffer."
		);

		// Semaphores for this frame
		VkSemaphore frameAvailable = sumiSwapChain->getImageAvailableSemaphore(currentFrameIdx);
		VkSemaphore renderFinished = sumiSwapChain->getRenderFinishedSemaphore(currentFrameIdx);
		VkSemaphore preComputeFinished = predrawComputeFinishedSemaphores[currentFrameIdx];
		VkSemaphore graphicsFinished = graphicsFinishedSemaphores[currentFrameIdx];
		VkSemaphore postComputeFinished = postComputeFinishedSemaphores[currentFrameIdx];

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

		// Submit pre-draw compute work
		// TODO: Check whether submitted to the async compute queue has any negative effect on post-compute.
		// TODO: If above comment is fine, we can combine the submits for pre and post compute to one 
		//         vkQueueSubmit call.
		VkPipelineStageFlags preComputeWaitStageFlag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkSubmitInfo preComputeSubmitInfo{};
		preComputeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		preComputeSubmitInfo.commandBufferCount = 1;
		preComputeSubmitInfo.pCommandBuffers = &frameCommandBuffers.predrawCompute;
		preComputeSubmitInfo.waitSemaphoreCount = 1;
		preComputeSubmitInfo.pWaitSemaphores = &frameAvailable;
		preComputeSubmitInfo.pWaitDstStageMask = &preComputeWaitStageFlag;
		preComputeSubmitInfo.signalSemaphoreCount = 1;
		preComputeSubmitInfo.pSignalSemaphores = &preComputeFinished;

		VK_CHECK_SUCCESS(
			vkQueueSubmit(sumiDevice.computeQueue(), 1, &preComputeSubmitInfo, nullptr),
			"[Sumire::SumiRenderer] Could not submit pre-draw compute command buffer."
		);

		// Submit main graphics work
		// TODO: We probably need to wait on at least the start of the fragment shader
		VkPipelineStageFlags graphicsWaitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo graphicsSubmitInfo{};
		graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		graphicsSubmitInfo.commandBufferCount = 1;
		graphicsSubmitInfo.pCommandBuffers = &frameCommandBuffers.graphics;
		graphicsSubmitInfo.waitSemaphoreCount = 1;
		graphicsSubmitInfo.pWaitSemaphores = &preComputeFinished;
		graphicsSubmitInfo.pWaitDstStageMask = &graphicsWaitStageFlag;
		graphicsSubmitInfo.signalSemaphoreCount = 1;
		graphicsSubmitInfo.pSignalSemaphores = &graphicsFinished;

		VK_CHECK_SUCCESS(
			vkQueueSubmit(sumiDevice.graphicsQueue(), 1, &graphicsSubmitInfo, nullptr),
			"[Sumire::SumiRenderer] Could not submit graphics command buffer."
		);

		// Submit post compute work
		VkPipelineStageFlags postComputeWaitStageFlag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkSubmitInfo postComputeSubmitInfo{};
		postComputeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		postComputeSubmitInfo.commandBufferCount = 1;
		postComputeSubmitInfo.pCommandBuffers = &frameCommandBuffers.compute;
		postComputeSubmitInfo.waitSemaphoreCount = 1;
		postComputeSubmitInfo.pWaitSemaphores = &graphicsFinished;
		postComputeSubmitInfo.pWaitDstStageMask = &postComputeWaitStageFlag;
		postComputeSubmitInfo.signalSemaphoreCount = 1;
		postComputeSubmitInfo.pSignalSemaphores = &postComputeFinished;

		VK_CHECK_SUCCESS(
			vkQueueSubmit(sumiDevice.computeQueue(), 1, &postComputeSubmitInfo, nullptr),
			"[Sumire::SumiRenderer] Could not submit post compute command buffer."
		);

		// Submit present composite work
		VkPipelineStageFlags compositeWaitStageFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		VkSubmitInfo compositeSubmitInfo{};
		compositeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		compositeSubmitInfo.commandBufferCount = 1;
		compositeSubmitInfo.pCommandBuffers = &frameCommandBuffers.present;
		compositeSubmitInfo.waitSemaphoreCount = 1;
		compositeSubmitInfo.pWaitSemaphores = &postComputeFinished;
		compositeSubmitInfo.pWaitDstStageMask = &compositeWaitStageFlag;
		compositeSubmitInfo.signalSemaphoreCount = 1;
		compositeSubmitInfo.pSignalSemaphores = &renderFinished;

		VK_CHECK_SUCCESS(
			vkQueueSubmit(sumiDevice.presentQueue(), 1, &compositeSubmitInfo, frameInFlightFence),
			"[Sumire::SumiRenderer] Could not submit present command buffer."
		);

		VkResult result = sumiSwapChain->queuePresent(&currentImageIdx, 1, &renderFinished);

		if (
			result == VK_ERROR_OUT_OF_DATE_KHR ||
			result == VK_SUBOPTIMAL_KHR || 
			sumiWindow.wasWindowResized()
		) {
			sumiWindow.resetWindowResizedFlag();
			recreateRenderObjects();
		} else if (result != VK_SUCCESS) {
            throw std::runtime_error("[Sumire::SumiRenderer] Failed to present swap chain image.");
        }

        isFrameStarted = false;
        currentFrameIdx = (currentFrameIdx + 1) % SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

	void SumiRenderer::beginRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Failed to begin scene render pass - no frame in flight.");
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
        renderPassInfo.framebuffer = framebuffers[currentFrameIdx];
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
				"[Sumire::SumiRenderer] Could not advance to next scene subpass - subpass index out of range.");
		}

		// TODO: Potentially change renderpass info 

		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	}

	void SumiRenderer::endRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Failed to end scene render pass - no frame in flight.");
        assert(commandBuffer == getCurrentCommandBuffers().graphics
			&& "Failed to end scene render pass: Command buffer provided did not record this renderpass."
        );

		// Note: We do not need to release the swapchain mirror to the compute queue
		//       as this is handled by the final subpass dependency.

        vkCmdEndRenderPass(commandBuffer);
	}

	void SumiRenderer::beginPostCompute(VkCommandBuffer commandBuffer) {
		// Compute is used to maximize graphics queue throughput.
		//   We can begin rendering the scene of a subsequent frame while we post-process the current frame.
		//   This has some concurrency implications - we may end up introducing compute pipeline bubbles
		//   from COMPUTE -> FRAGMENT barriers if we are not careful about how we submit work to VkQueues.
		//   TLDR; Separate queues in a ring-like fashion to ensure we can interleave compute & fragment work.
		// More information:
		//   https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/async_compute
		//   https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/using-compute-post-processing-in-vulkan-on-mali

		// This function is (currently) a placeholder to standardize the SumiRenderer compute interface
		//  with the renderpass interface.

		// Note: We do not need to acquire the swapchain mirror attachment here as it is made available
		//       by the final subpass dependency from the scene renderpass.
	}

	void SumiRenderer::endPostCompute(VkCommandBuffer commandBuffer) {
		SumiAttachment* currentSwapchainMirrorAttachment = intermediateColorAttachments[currentFrameIdx].get();

		// Release image due for swapchain (pending composite)
		uint32_t computeFamilyIdx = sumiDevice.computeQueueFamilyIndex();
		uint32_t presentFamilyIdx = sumiDevice.presentQueueFamilyIndex();

		if (computeFamilyIdx != presentFamilyIdx) {
			sumiDevice.imageMemoryBarrier(
				currentSwapchainMirrorAttachment->getImage(),
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL,
				0,
				0,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				currentSwapchainMirrorAttachment->getImageViewInfo().subresourceRange,
				computeFamilyIdx, // Compute queue
				presentFamilyIdx, // (High Priority) Present queue
				commandBuffer
			);
		}
	}

	void SumiRenderer::beginCompositeRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Failed to begin composite render pass - no frame in flight.");

		currentSubpass = 0;

		// Acquire image from compute
		SumiAttachment* currentSwapchainMirrorAttachment = intermediateColorAttachments[currentFrameIdx].get();
		uint32_t computeFamilyIdx = sumiDevice.computeQueueFamilyIndex();
		uint32_t presentFamilyIdx = sumiDevice.presentQueueFamilyIndex();

		if (computeFamilyIdx != presentFamilyIdx) {
			sumiDevice.imageMemoryBarrier(
				currentSwapchainMirrorAttachment->getImage(),
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL,
				0,
				0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				currentSwapchainMirrorAttachment->getImageViewInfo().subresourceRange,
				computeFamilyIdx, // Compute Queue
				presentFamilyIdx, // Present Queue
				commandBuffer
			);
		}

		// Begin renderpass for final frame composition
		std::array<VkClearValue, 1> attachmentClearValues{};
		attachmentClearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = compositionRenderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[currentImageIdx]; // Render to current swapchain image
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

	void SumiRenderer::endCompositeRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Failed to end composite render pass - no frame in flight.");
		assert(commandBuffer == getCurrentCommandBuffers().present
			&& "Failed to end composite render pass: Command buffer provided did not record this renderpass."
		);

		vkCmdEndRenderPass(commandBuffer);
	}
}