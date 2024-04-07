#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <sumire/util/vk_check_success.hpp>

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

namespace sumire {

	// local callback functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	// class member functions
	SumiDevice::SumiDevice(SumiWindow& window) : window{ window } {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
	}

	SumiDevice::~SumiDevice() {
		// Note this encapsulates destroying components of the device: command pool, etc.
		vkDestroyCommandPool(device_, commandPool, nullptr);
		vkDestroyDevice(device_, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface_, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void SumiDevice::createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("[Sumire::SumiDevice] Validation layers were requested, but are not available.");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Sumire";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "Sumire";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

			std::cout << "[Sumire::SumiDevice] Validation layers are ENABLED" << std::endl;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;

			std::cout << "[Sumire::SumiDevice] Validation layers are DISABLED" << std::endl;
		}

		VK_CHECK_SUCCESS(
			vkCreateInstance(&createInfo, nullptr, &instance),
			"[Sumire::SumiDevice] Failed to create a vulkan instance."
		);
		hasGflwRequiredInstanceExtensions();
	}

	void SumiDevice::pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("[Sumire::SumiDevice] Failed to find GPUs with Vulkan support.");
		}
		std::cout << "[Sumire::SumiDevice] Checking for GPU support among " << deviceCount << " physical device(s)..." << std::endl;
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("[Sumire::SumiDevice] Failed to find a suitable GPU from candidates.");
		}

		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		std::cout << "[Sumire::SumiDevice] Using physical device: " << properties.deviceName << std::endl;
	}

	void SumiDevice::createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		if (!indices.hasDedicatedComputeFamily) {
			std::cout << "[Sumire::SumiDevice] WARNING: Physical device has no dedicated compute queue - using a combined compute queue instead." << std::endl;
		}
		if (!indices.hasDedicatedTransferFamily) {
			std::cout << "[Sumire::SumiDevice] WARNING: Physical device has no dedicated transfer queue - using a combined transfer queue instead." << std::endl;
		}

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Enable descriptor indexing features
		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
		descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;

		// TODO: Should check for feature support and log a warning if not supported.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &descriptorIndexingFeatures; // pass indexing features to device create info

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// might not really be necessary anymore because device specific validation layers
		// have been deprecated
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK_SUCCESS(
			vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_),
			"[Sumire::SumiDevice] Failed to create logical device."
		);

		vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
		vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
	}

	void SumiDevice::createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK_SUCCESS(
			vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool),
			"[Sumire::SumiDevice] Failed to create command pool."
		);
	}

	void SumiDevice::createSurface() { window.createWindowSurface(instance, &surface_); }

	bool SumiDevice::isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.hasValidQueueSupport() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
	}

	void SumiDevice::populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;  // Optional
	}

	void SumiDevice::setupDebugMessenger() {
		if (!enableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		VK_CHECK_SUCCESS(
			CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
			"[Sumire::SumiDevice] Failed to set up debug messenger."
		);
	}

	bool SumiDevice::checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> SumiDevice::getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void SumiDevice::hasGflwRequiredInstanceExtensions() {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "available extensions:" << std::endl;
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
			available.insert(extension.extensionName);
		}

		std::cout << "required extensions:" << std::endl;
		auto requiredExtensions = getRequiredExtensions();
		for (const auto& required : requiredExtensions) {
			std::cout << "\t" << required << std::endl;
			if (available.find(required) == available.end()) {
				throw std::runtime_error("[Sumire::SumiDevice] Missing required glfw extension.");
			}
		}
	}

	bool SumiDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices SumiDevice::findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// Query physical device queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		
		// Generally speaking, we can make use of 3 queue types for most graphics tasks on modern hardware:
		//  - Combined Graphics & Compute queues
		//  - Dedicated Compute Queues
		//  - Dedicated Transfer Queues

		// Sumire is set up to run best with async compute dispatches.
		//  Some (particularly old / embedded) hardware does not have a dedicated compute queue.
		//  In such cases, we have to force all the the work linearly through a combined graphics/compute queue.

		// Additionally having prioritised graphics queues is beneficial for frame throughput
		//  (e.g. when we want to quickly finish off a frame for presenting when also starting to rasterize another).
		//  Thus having *at least two* graphics queues available is useful for this separation.

		// First pass: Try and find all desired queue types exactly.
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); i++) {
			if (indices.hasValidQueueSupport()) break; // short circuit if required queues are found

			VkQueueFamilyProperties& currentProperties = queueFamilies[i];
			if (currentProperties.queueCount <= 0) continue;

			// Present Family
			if (!indices.hasValidPresentFamily) {
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
				if (presentSupport) {
					indices.presentFamily = i;
					indices.hasValidPresentFamily = true;
				}
			}

			// Graphics Family
			if (!indices.hasValidGraphicsFamily &&
				currentProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT
			) {
				indices.graphicsFamily = i;
				indices.hasValidGraphicsFamily = true;
			}

			// Dedicated Compute Family
			if (!indices.hasValidComputeFamily &&
				currentProperties.queueFlags & VK_QUEUE_COMPUTE_BIT && 
				!(currentProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			) {
				indices.dedicatedComputeFamily = i;
				indices.hasValidComputeFamily = true;
				indices.hasDedicatedComputeFamily = true;
			}

			// Dedicated Transfer Family
			if (!indices.hasValidTransferFamily &&
				currentProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
				!(currentProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				!(currentProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
			) {
				indices.dedicatedTransferFamily = i;
				indices.hasValidTransferFamily = true;
				indices.hasDedicatedTransferFamily = true;
			}
		}

		// Deal with cases where dedicated queues are not present.
		if (!indices.hasValidComputeFamily) {
			// Find any applicable compute queue to use instead.
			indices.hasValidComputeFamily = findFirstValidQueueFamily(
				device, 
				VK_QUEUE_COMPUTE_BIT, 
				indices.dedicatedComputeFamily
			);
		}
		if (!indices.hasValidTransferFamily) {
			indices.hasValidTransferFamily = findFirstValidQueueFamily(
				device,
				VK_QUEUE_TRANSFER_BIT, 
				indices.dedicatedTransferFamily
			);
		}

		return indices;
	}

	bool SumiDevice::findFirstValidQueueFamily(VkPhysicalDevice device, VkQueueFlags flags, uint32_t& idx) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		bool foundValidQueueFamily = false;
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); i++) {
			VkQueueFamilyProperties& currentProperties = queueFamilies[i];
			if (currentProperties.queueCount <= 0) continue;

			if (currentProperties.queueFlags & flags) {
				idx = i;
				foundValidQueueFamily = true;
			}
		}

		return foundValidQueueFamily;
	}

	SwapChainSupportDetails SumiDevice::querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface_,
				&presentModeCount,
				details.presentModes.data());
		}
		return details;
	}

	VkFormat SumiDevice::findSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (
				tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("[Sumire::SumiDevice] Failed to find supported format from candidates.");
	}

	uint32_t SumiDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("[Sumire::SumiDevice] Failed to find suitable memory type requested.");
	}

	void SumiDevice::createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_SUCCESS(
			vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer),
			"[Sumire::SumiDevice] Failed to create requested buffer."
		);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		VK_CHECK_SUCCESS(
			vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory),
			"[Sumire::SumiDevice] Failed to allocate memory for requested buffer."
		);

		vkBindBufferMemory(device_, buffer, bufferMemory, 0);
	}

	VkCommandBuffer SumiDevice::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void SumiDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
	}

	void SumiDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		// To avoid this single time wait, use a memory barrier instead.
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;  // Optional
		copyRegion.dstOffset = 0;  // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void SumiDevice::copyBufferToImage(
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		endSingleTimeCommands(commandBuffer);
	}

	void SumiDevice::createImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
	) {
		
		VK_CHECK_SUCCESS(
			vkCreateImage(device_, &imageInfo, nullptr, &image),
			"[Sumire::SumiDevice] Failed to create requested image."
		);

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device_, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		VK_CHECK_SUCCESS(
			vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory),
			"[Sumire::SumiDevice] Failed to allocate memory for requested image."
		);
		VK_CHECK_SUCCESS(
			vkBindImageMemory(device_, image, imageMemory, 0),
			"[Sumire::SumiDevice] Failed to bind memory for requested image."
		);
	}

	void SumiDevice::imageMemoryBarrier(
		VkImage image,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkAccessFlags srcAcessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange,
		uint32_t srcQueueFamilyIndex,
		uint32_t dstQueueFamilyIndex,
		VkCommandBuffer commandBuffer
	) {
		bool needsCommandBuffer = (commandBuffer == VK_NULL_HANDLE);
		if (needsCommandBuffer) commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAcessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStageMask,
			dstStageMask,
			0x0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (needsCommandBuffer) endSingleTimeCommands(commandBuffer);
	}

	void SumiDevice::transitionImageLayout(
		VkImage image, 
		VkImageSubresourceRange subresourceRange, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout,
		VkCommandBuffer commandBuffer
	) {
		bool needsCommandBuffer = (commandBuffer == VK_NULL_HANDLE);
		if (needsCommandBuffer) commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		
		// Do not transfer queue family ownership
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		barrier.subresourceRange = subresourceRange;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		switch (oldLayout) {

		// UNDEFINED -> X
		case VK_IMAGE_LAYOUT_UNDEFINED: {
			switch (newLayout) {
				
			// UNDEFINED -> TRANSFER_DST_OPTIMAL
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;	
			}
			break;

			// UNDEFINED -> TRANSFER_SRC_OPTIMAL
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			break;

			default:
				throw std::invalid_argument("[Sumire::SumiDevice] Unsupported Image Transition: LAYOUT_UNDEFINED -> LAYOUT ID [" 
					+ std::to_string(newLayout) + "]");
			}
		}
		break;

		// TRANSFER_SRC_OPTIMAL -> X
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
			switch (newLayout) {

			// TRANSFER_SRC_OPTIMAL -> TRANSFER_DST_OPTIMAL
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			break;

			// TRANSFER_SRC_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // TODO: only fragment shader for now
			}
			break;

			default:
				throw std::invalid_argument("[Sumire::SumiDevice] Unsupported Image Transition: LAYOUT_TRASNFER_SRC_OPTIMAL -> LAYOUT ID ["
					+ std::to_string(newLayout) + "]");
			}
		}
		break;

		// TRANSFER_DST_OPTIMAL -> X
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
			switch (newLayout) {

			// TRANSFER_DST_OPTIMAL -> TRANSFER_SRC_OPTIMAL
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			break;

			// TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // TODO: only fragment shader for now
			}
			break;

			default: 
				throw std::invalid_argument("[Sumire::SumiDevice] Unsupported Image Transition: LAYOUT_TRANSFER_DST_OPTIMAL -> LAYOUT ID [" 
					+ std::to_string(newLayout) + "]");

			}
		}
		break;

		default:
			throw std::invalid_argument("[Sumire::SumiDevice] Unsupported Image Transition: LAYOUT ID ["
				+ std::to_string(oldLayout) + "] -> LAYOUT ID [" + std::to_string(newLayout) + "]");

		}

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStage, dstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (needsCommandBuffer) endSingleTimeCommands(commandBuffer);
	}

}