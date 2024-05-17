#pragma once

#include <vulkan/vulkan.h>

#include <sumire/config/sumi_config.hpp>
#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/shaders/shader_manager.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>

namespace sumire {

    struct PhysicalDeviceDetails {
        std::string name;
        VkDeviceSize localMemorySize;
        uint32_t idx;
        bool suitable;
        bool discrete;
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        uint32_t presentFamily;
        uint32_t graphicsFamily;
        uint32_t computeFamily;
        uint32_t transferFamily;

        bool hasValidPresentFamily = false;

        bool hasValidGraphicsFamily = false;
        bool hasMultipleGraphicsQueues = false;

        bool hasValidComputeFamily = false;
        bool hasDedicatedComputeFamily = false;

        bool hasValidTransferFamily = false;
        bool hasDedicatedTransferFamily = false;

        bool hasValidQueueSupport() const { 
            return (
                hasValidPresentFamily && hasValidGraphicsFamily &&
                hasValidComputeFamily && hasValidTransferFamily
            );
        }
    };

    class SumiDevice {
    public:

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

        SumiDevice(
            SumiWindow& window,
            SumiConfig* config = nullptr
        );
        ~SumiDevice();

        SumiDevice(const SumiDevice&) = delete;
        SumiDevice& operator=(const SumiDevice&) = delete;
        SumiDevice(SumiDevice&&) = delete;
        SumiDevice& operator=(SumiDevice&&) = delete;

        VkDevice device() const { return device_; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        PhysicalDeviceDetails getPhysicalDeviceDetails() const { return physicalDeviceDetails; }
        const std::vector<PhysicalDeviceDetails>& getPhysicalDeviceList() const { 
            return physicalDeviceList; 
        }

        VkInstance getInstance() const { return instance; }
        VkSurfaceKHR surface() const { return surface_; }

        VkQueue graphicsQueue() const { return graphicsQueue_; }
        uint32_t graphicsQueueFamilyIndex() const { return queueFamilyIndices.graphicsFamily; }
        VkQueue computeQueue() const { return computeQueue_; }
        uint32_t computeQueueFamilyIndex() const { return queueFamilyIndices.computeFamily; }
        VkQueue presentQueue() const { return presentQueue_; }
        uint32_t presentQueueFamilyIndex() const { return queueFamilyIndices.presentFamily; }
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        VkCommandPool getGraphicsCommandPool() const { return graphicsCommandPool; }
        VkCommandPool getPresentCommandPool() const { return presentCommandPool; }
        VkCommandPool getComputeCommandPool() const {
            return computeCommandPool == VK_NULL_HANDLE ? graphicsCommandPool : computeCommandPool; }
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
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
            VkDeviceMemory& imageMemory
        );
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
        void pickPhysicalDevice(SumiConfig* config);
        void createLogicalDevice();
        void initShaderManager();
        void createCommandPools();
        void writeDeviceInfoToConfig(SumiConfig* config);

        VkDeviceSize getLocalHeapSize(
            const VkPhysicalDeviceMemoryProperties& memoryProperties) const;
        bool supportedFeaturesAreSuitable(
            const VkPhysicalDeviceFeatures& supportedFeautres) const;
        bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char*> getRequiredExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool checkValidationLayerSupport();
        void hasGflwRequiredInstanceExtensions();
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        // Queue Family querying
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool findFirstValidQueueFamily(VkPhysicalDevice device, VkQueueFlags flags, uint32_t& idx);

        SumiWindow& window;
        //std::unique_ptr<ShaderManager> shaderManager;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        VkDevice device_;

        VkCommandPool graphicsCommandPool;
        VkCommandPool presentCommandPool;
        VkCommandPool computeCommandPool = VK_NULL_HANDLE;

        VkSurfaceKHR surface_;
        QueueFamilyIndices queueFamilyIndices;
        VkQueue graphicsQueue_;
        VkQueue computeQueue_;
        VkQueue presentQueue_;

        // this should be a static member if we ever want more than one SumiDevice
        std::vector<PhysicalDeviceDetails> physicalDeviceList{};
        PhysicalDeviceDetails physicalDeviceDetails;

        const std::vector<const char*> validationLayers = { 
            "VK_LAYER_KHRONOS_validation" 
        };
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
            "VK_EXT_descriptor_indexing"
        };

};

}