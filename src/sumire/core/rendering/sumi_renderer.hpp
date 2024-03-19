#pragma once

#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/rendering/sumi_swap_chain.hpp>
#include <sumire/core/rendering/sumi_gbuffer.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <memory>
#include <vector>
#include <cassert>

namespace sumire {

	class SumiRenderer {
	public:
		SumiRenderer(SumiWindow& window, SumiDevice& device);
		~SumiRenderer();

		SumiRenderer(const SumiRenderer&) = delete;
		SumiRenderer& operator=(const SumiRenderer&) = delete;

        SumiWindow& getWindow() const { return sumiWindow; }
        SumiDevice& getDevice() const { return sumiDevice; }

        VkRenderPass getSwapChainRenderPass() const { return sumiSwapChain->getRenderPass(); }
        float getAspect() const { return sumiSwapChain->extentAspectRatio(); }
        VkFormat getSwapChainImageFormat () const { return sumiSwapChain->getSwapChainImageFormat(); }

        bool wasSwapChainRecreated() const { return scRecreatedFlag; }
        void resetScRecreatedFlag() { scRecreatedFlag = false; }

        bool isFrameInProgress() const { return isFrameStarted; }
        
        VkCommandBuffer getCurrentCommandBuffer() const { 
            assert(isFrameStarted && "Failed to get command buffer from a frame not in flight.");
            return commandBuffers[currentFrameIdx];
        }
        VkCommandBuffer getCurrentDeferredCommandBuffer() const {
            assert(isFrameStarted && "Failed to get deferred command buffer - no frame is in flight");
            return deferredCommandBuffers[currentFrameIdx];
        }

        int getFrameIdx() const {
            assert(isFrameStarted && "Failed to get frame index (frame not in flight).");
            return currentFrameIdx;
        }

        // Recording of command buffers
        VkCommandBuffer beginFrame();
        void endFrame();

        void beginGbufferRenderPass(VkCommandBuffer commandBuffer);
        void endGbufferRenderPass(VkCommandBuffer commandBuffer);

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();
        void recreateGbuffer();

        bool scRecreatedFlag{false};

		SumiWindow& sumiWindow;
		SumiDevice& sumiDevice;

        // Swap chain
		std::unique_ptr<SumiSwapChain> sumiSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

        // Offscreen Deferred Rendering
        std::unique_ptr<SumiGbuffer> gbuffer;
        // We need multiple command buffers for recording per swapchain image,
        //   but only one gbuffer due to execution dependency of the deferred renderpass.
        std::vector<VkCommandBuffer> deferredCommandBuffers;

        uint32_t currentImageIdx;
        int currentFrameIdx{0};
        bool isFrameStarted{false};
	};
}