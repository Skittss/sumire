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

        struct FrameCommandBuffers {
            VkCommandBuffer deferred = VK_NULL_HANDLE;
            VkCommandBuffer swapChain = VK_NULL_HANDLE;

            bool validFrame() const {
                return (deferred && swapChain);
            }
        };

        bool isFrameInProgress() const { return isFrameStarted; }
        
        FrameCommandBuffers getCurrentCommandBuffers() const;

        int getFrameIdx() const {
            assert(isFrameStarted && "Failed to get frame index (frame not in flight).");
            return currentFrameIdx;
        }

        // Recording of command buffers
        FrameCommandBuffers beginFrame();
        void endFrame();

        // Deffered renderpass data
        void beginGbufferRenderPass(VkCommandBuffer commandBuffer);
        void endGbufferRenderPass(VkCommandBuffer commandBuffer);

        VkRenderPass getGbufferRenderPass() const { return gbuffer->getRenderPass(); }
        SumiGbuffer* getGbuffer() const { return gbuffer.get(); }

        // Swapchain renderpass data
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        VkRenderPass getSwapChainRenderPass() const { return sumiSwapChain->getRenderPass(); }
        float getAspect() const { return sumiSwapChain->extentAspectRatio(); }
        VkFormat getSwapChainImageFormat () const { return sumiSwapChain->getSwapChainImageFormat(); }
        bool wasSwapChainRecreated() const { return scRecreatedFlag; }
        void resetScRecreatedFlag() { scRecreatedFlag = false; }

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