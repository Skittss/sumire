#include <sumire/core/rendering/sumi_swap_chain.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace sumire {

	SumiSwapChain::SumiSwapChain(SumiDevice& deviceRef, VkExtent2D extent
	) : device{ deviceRef }, windowExtent{ extent } {
		init();
	}

	SumiSwapChain::SumiSwapChain(
		SumiDevice& deviceRef, 
		VkExtent2D extent, 
		std::shared_ptr<SumiSwapChain> previous
	) : device{ deviceRef }, windowExtent{ extent }, oldSwapChain{ previous } {
		init();

		// Only use old swap chain ptr to init a new one to potentially save resources
		oldSwapChain = nullptr;
	}

	void SumiSwapChain::init() {
		createSwapChain();
		createAttachments();
		createSyncObjects();
	}

	SumiSwapChain::~SumiSwapChain() {
		depthAttachment = nullptr;
		colorAttachments.clear();

		if (swapChain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
			swapChain = VK_NULL_HANDLE;
		}

		// cleanup synchronization objects
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
		}
	}

	VkResult SumiSwapChain::acquireNextImage(uint32_t currentFrameIdx, uint32_t* imageIdx) {
		vkWaitForFences(
			device.device(),
			1,
			&inFlightFences[currentFrameIdx],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max());

		VkResult result = vkAcquireNextImageKHR(
			device.device(),
			swapChain,
			std::numeric_limits<uint64_t>::max(),
			imageAvailableSemaphores[currentFrameIdx],  // must be a not signaled semaphore
			VK_NULL_HANDLE,
			imageIdx);

		return result;
	}

	VkResult SumiSwapChain::queuePresent(
		uint32_t* imageIndex,
		uint32_t waitSemaphoreCount,
		VkSemaphore* pWaitSemaphores
	) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = imageIndex;

		// Make sure to wait until specified image is done rendering before presenting.
		presentInfo.waitSemaphoreCount = waitSemaphoreCount;
		presentInfo.pWaitSemaphores = pWaitSemaphores;

		return vkQueuePresentKHR(device.presentQueue(), &presentInfo);
	}

	void SumiSwapChain::createSwapChain() {
		SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.surface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

		VK_CHECK_SUCCESS(
			vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain),
			"[Sumire::SumiSwapChain] Failed to create swap chain."
		);

		colorFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void SumiSwapChain::createAttachments() {
		assert(swapChain != VK_NULL_HANDLE 
			&& "Cannot create swap chain attachments before creating the swap chain itself.");
		// Color attachments
		//   First query how many images we have
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
		assert(imageCount > 0 && "Swap chain image query returned an invalid number of image handles.");

		//   Get the image handles
		std::vector<VkImage> swapChainImageHandles(imageCount);
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImageHandles.data());

		//   Create attachments from the image handles
		colorAttachments = std::vector<std::unique_ptr<SumiAttachment>>(imageCount);
		for (uint32_t i = 0; i < colorAttachments.size(); i++) {
			colorAttachments[i] = std::make_unique<SumiAttachment>(
				device,
				swapChainExtent,
				swapChainImageHandles[i],
				colorFormat,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		// Depth attachment
		VkFormat depthAttachmentFormat = findDepthFormat();
		depthFormat = depthAttachmentFormat;
		VkExtent2D swapChainExtent = getExtent();

		depthAttachment = std::make_unique<SumiAttachment>(
			device,
			swapChainExtent,
			depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	void SumiSwapChain::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VK_CHECK_SUCCESS(
				vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]),
				"[Sumire::SumiSwapChain] Failed to create swap chain wait semaphores."
			);
			VK_CHECK_SUCCESS(
				vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]),
				"[Sumire::SumiSwapChain] Failed to create swap chain signaling semaphores."
			);
			VK_CHECK_SUCCESS(
				vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]),
				"[Sumire::SumiSwapChain] Failed to create swap chain frame fences."
			);
		}
	}

	VkSurfaceFormatKHR SumiSwapChain::chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR SumiSwapChain::chooseSwapPresentMode(
		// Default Mailbox; GPU never idles and does not wait for Vsync, instead overwrites old buffers
		// FIFO can be used to force a wait for the Vsync, this will introduce idling time on the GPU.
		// These are both Vsync approaches, i.e. no screen tearing.
		const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "[Sumire::SumiSwapChain] Swapped to present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}

		// Immediate mode does not wait for a vsync to update buffers.
		// for (const auto &availablePresentMode : availablePresentModes) {
		//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
		//     std::cout << "Present mode: Immediate" << std::endl;
		//     return availablePresentMode;
		//   }
		// }

		std::cout << "[Sumire::SumiSwapChain] Swapped to present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SumiSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = windowExtent;
			actualExtent.width = std::max(
				capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(
				capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VkFormat SumiSwapChain::findDepthFormat() {
		return device.findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

}
