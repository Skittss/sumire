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
            VkCommandBuffer predrawCompute = VK_NULL_HANDLE;
            VkCommandBuffer graphics = VK_NULL_HANDLE;
            VkCommandBuffer compute = VK_NULL_HANDLE;
            VkCommandBuffer present = VK_NULL_HANDLE;

            bool validFrame() const {
                return (predrawCompute && graphics && compute && present);
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

        // Scene Renderpass
        //  The bulk of traditional graphics rendering (e.g. gbuffer fill & resolve)
        void beginRenderPass(VkCommandBuffer commandBuffer);
        void nextSubpass(VkCommandBuffer commandBuffer);
        void endRenderPass(VkCommandBuffer commandBuffer);
        VkRenderPass getRenderPass() const { return renderPass; }

        // Post Renderpass
        //  With subpasses responsible for HDR + Bloom, tonemapping and UI.
        void beginPostCompute(VkCommandBuffer commandBuffer);
        void endPostCompute(VkCommandBuffer commandBuffer);
        void beginCompositeRenderPass(VkCommandBuffer commandBuffer);
        void endCompositeRenderPass(VkCommandBuffer commandbuffer);
        VkRenderPass getCompositionRenderPass() const { return compositionRenderPass; }

        SumiGbuffer* getGbuffer() const { return gbuffer.get(); }
        uint32_t gbufferFillSubpassIdx() const { return 0; }
        uint32_t gbufferResolveSubpassIdx() const { return 1; }
        uint32_t forwardRenderSubpassIdx() const { return 2; }
        uint32_t compositionSubpassIdx() const { return 0; }
        bool wasGbufferRecreated() const { return gbufferRecreatedFlag; }
        void resetGbufferRecreatedFlag() { gbufferRecreatedFlag = false; }

        std::vector<SumiAttachment*> getIntermediateColorAttachments() const;
        VkFormat getIntermediateColorAttachmentFormat() const;

        float getAspect() const { return sumiSwapChain->getAspectRatio(); }
        VkFormat getSwapChainColorFormat () const { return sumiSwapChain->getColorFormat(); }
        bool wasSwapChainRecreated() const { return scRecreatedFlag; }
        void resetScRecreatedFlag() { scRecreatedFlag = false; }

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
        void recreateRenderObjects();
		void recreateSwapChain();
        void recreateGbuffer();

        void createRenderPass();
        void createCompositionRenderPass();
        void createFramebuffers();
        void freeFramebuffers();

        void createSyncObjects();
        void freeSyncObjects();

        bool scRecreatedFlag = false;
        bool gbufferRecreatedFlag = false;

		SumiWindow& sumiWindow;
		SumiDevice& sumiDevice;

        std::vector<VkSemaphore> predrawComputeFinishedSemaphores;
        std::vector<VkSemaphore> graphicsFinishedSemaphores;
        std::vector<VkSemaphore> postComputeFinishedSemaphores;

        // Graphics render pass
        uint32_t currentSubpass = 0;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        // Post render pass
        VkRenderPass compositionRenderPass = VK_NULL_HANDLE;

        // Command buffers
        std::vector<VkCommandBuffer> predrawComputeCommandBuffers;
		std::vector<VkCommandBuffer> graphicsCommandBuffers;
        std::vector<VkCommandBuffer> computeCommandBuffers;
        std::vector<VkCommandBuffer> presentCommandBuffers;

        // Frame buffers
        std::vector<VkFramebuffer> swapchainFramebuffers; // TODO: maybe move this to swapchain class.
        std::vector<VkFramebuffer> framebuffers;

        // Swap chain
		std::unique_ptr<SumiSwapChain> sumiSwapChain;

        // Offscreen Deferred Rendering Targets
        std::unique_ptr<SumiGbuffer> gbuffer;

        // Offscreen Intermediate Color Targets (Swap Chain Mirror)
        std::vector<std::unique_ptr<SumiAttachment>> intermediateColorAttachments;

        uint32_t currentImageIdx = 0; // for return value of vkAcquireNextImageKHR
        uint32_t currentFrameIdx = 0;
        bool isFrameStarted = false;
	};
}