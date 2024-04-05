#pragma once

#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/rendering/sumi_swap_chain.hpp>
#include <sumire/core/rendering/sumi_gbuffer.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

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
            VkCommandBuffer swapChain = VK_NULL_HANDLE;

            bool validFrame() const {
                return (swapChain);
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

        // RenderPass
        void beginRenderPass(VkCommandBuffer commandBuffer);
        void nextSubpass(VkCommandBuffer commandBuffer);
        void endRenderPass(VkCommandBuffer commandBuffer);
        VkRenderPass getRenderPass() const { return renderPass; }

        SumiGbuffer* getGbuffer() const { return gbuffer.get(); }
        uint32_t gbufferFillSubpassIdx() const { return 0; }
        uint32_t gbufferResolveSubpassIdx() const { return 1; }
        uint32_t forwardRenderSubpassIdx() const { return 2; }

        float getAspect() const { return sumiSwapChain->getAspectRatio(); }
        VkFormat getSwapChainColorFormat () const { return sumiSwapChain->getColorFormat(); }
        bool wasSwapChainRecreated() const { return scRecreatedFlag; }
        void resetScRecreatedFlag() { scRecreatedFlag = false; }
        bool wasGbufferRecreated() const { return gbufferRecreatedFlag; }
        void resetGbufferRecreatedFlag() { gbufferRecreatedFlag = false; }

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
        void recreateRenderObjects();
		void recreateSwapChain();
        void recreateGbuffer();

        void createRenderPass();
        void createFramebuffers();
        void freeFramebuffers();

        bool scRecreatedFlag = false;
        bool gbufferRecreatedFlag = false;

		SumiWindow& sumiWindow;
		SumiDevice& sumiDevice;

        uint32_t currentSubpass = 0;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        std::vector<VkFramebuffer> framebuffers;

        // Swap chain
		std::unique_ptr<SumiSwapChain> sumiSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

        // Offscreen Deferred Rendering
        std::unique_ptr<SumiGbuffer> gbuffer;
        // We need multiple command buffers for recording per swapchain image,
        //   but only one gbuffer due to execution dependency of the deferred renderpass.
        std::vector<VkCommandBuffer> deferredCommandBuffers;

        uint32_t currentImageIdx = 0; // for return value of vkAcquireNextImageKHR
        uint32_t currentFrameIdx = 0;
        bool isFrameStarted = false;
	};
}