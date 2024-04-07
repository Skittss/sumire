#pragma once

#include <sumire/core/windowing/sumi_window.hpp>

#include <string>
#include <vector>

//#ifndef NDEBUG
//#define NDEBUG
//#endif

namespace sumire {

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t computeFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool computeFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() { return graphicsFamilyHasValue && computeFamilyHasValue && presentFamilyHasValue; }
	};

	class SumiDevice {
	public:

	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif

		SumiDevice(SumiWindow& window);
		~SumiDevice();

		// Not copyable or movable
		SumiDevice(const SumiDevice&) = delete;
		SumiDevice& operator=(const SumiDevice&) = delete;
		SumiDevice(SumiDevice&&) = delete;
		SumiDevice& operator=(SumiDevice&&) = delete;

		// TODO: These get functions are all over the place name-wise and should be const.
		VkCommandPool getCommandPool() const { return commandPool; }
		VkDevice device() const { return device_; }
		VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
		VkInstance getInstance() const { return instance; }
		VkSurfaceKHR surface() const { return surface_; }
		VkQueue graphicsQueue() const { return graphicsQueue_; }
		VkQueue presentQueue() const { return presentQueue_; }

		SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer Helper Functions
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory
		);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(
			VkBuffer buffer, VkImage image, 
			uint32_t width, uint32_t height, 
			uint32_t layerCount
		);

		// Image helpers
		void createImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);
		void imageMemoryBarrier(
			VkImage image,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkAccessFlags srcAcessMask,
			VkAccessFlags dstAccessMask,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange,
			uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE
		);
		void transitionImageLayout(
			VkImage image, 
			VkImageSubresourceRange subresourceRange,
			VkImageLayout oldLayout, 
			VkImageLayout newLayout,
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE // Optionally provide single-time command buffer
		);

		VkPhysicalDeviceProperties properties;

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		SumiWindow& window;
		VkCommandPool commandPool;

		VkDevice device_;
		VkSurfaceKHR surface_;
		VkQueue graphicsQueue_;
		VkQueue presentQueue_;

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};

}