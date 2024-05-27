#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

namespace sumire {

    class SumiSwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SumiSwapChain(
            SumiDevice& deviceRef,
            VkExtent2D windowExtent, 
            bool vsync = false
        );
        SumiSwapChain(
            SumiDevice& deviceRef,
            VkExtent2D windowExtent,
            std::shared_ptr<SumiSwapChain> previous,
            bool vsync = false
        );
        ~SumiSwapChain();

        SumiSwapChain(const SumiSwapChain&) = delete;
        SumiSwapChain& operator=(const SumiSwapChain&) = delete;

        size_t imageCount() { return colorAttachments.size(); }
        bool isVsyncEnabled() { return vsyncEnabled; }
        
        SumiAttachment* getColorAttachment(uint32_t idx) const { return colorAttachments[idx].get(); }
        SumiAttachment* getDepthAttachment() const { return depthAttachment.get(); }

        VkFormat getColorFormat() const { return colorFormat; }
        VkFormat getDepthFormat() const { return depthFormat; }
        VkFormat findDepthFormat();

        VkExtent2D getExtent() const { return swapChainExtent; }
        uint32_t width() const { return swapChainExtent.width; }
        uint32_t height() const { return swapChainExtent.height; }
        float getAspectRatio() const { 
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height); }

        VkResult acquireNextImage(uint32_t currentFrameIdx, uint32_t* imageIndex);
        VkResult queuePresent(uint32_t* imageIndex, uint32_t waitSemaphoreCount, VkSemaphore* pWaitSemaphore);

        bool compareSwapFormats(const SumiSwapChain& swapChain) const {
            return swapChain.depthFormat == depthFormat && 
                   swapChain.colorFormat == colorFormat;
        }

        // Expose synchronisation objects
        VkFence getImageInFlightFence(uint32_t idx) const { return imagesInFlight[idx]; }
        void setImageInFlightFence(uint32_t imageIdx, uint32_t frameIdx) { 
            imagesInFlight[imageIdx] = inFlightFences[frameIdx];
        }

        VkSemaphore getImageAvailableSemaphore(uint32_t idx) const { return imageAvailableSemaphores[idx]; }
        VkSemaphore getRenderFinishedSemaphore(uint32_t idx) const { return renderFinishedSemaphores[idx]; }

    private:
        void init();
        void createSwapChain();
        void createAttachments(); 
        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkExtent2D swapChainExtent;

        VkFormat colorFormat;
        VkFormat depthFormat;
        std::vector<std::unique_ptr<SumiAttachment>> colorAttachments;
        std::unique_ptr<SumiAttachment> depthAttachment;

        SumiDevice& device;
        VkExtent2D windowExtent;
        bool vsyncEnabled;

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::shared_ptr<SumiSwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
    };

}
