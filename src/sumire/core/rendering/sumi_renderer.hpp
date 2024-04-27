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
        SumiRenderer(SumiWindow& window, SumiDevice& device, SumiConfig& config);
        ~SumiRenderer();

        SumiRenderer(const SumiRenderer&) = delete;
        SumiRenderer& operator=(const SumiRenderer&) = delete;

        SumiWindow& getWindow() const { return sumiWindow; }
        SumiDevice& getDevice() const { return sumiDevice; }
        SumiSwapChain* getSwapChain() const { return sumiSwapChain.get(); }

        struct FrameCommandBuffers {
            // Command buffers are submitted in the following order
            VkCommandBuffer predrawCompute = VK_NULL_HANDLE;
            VkCommandBuffer earlyGraphics  = VK_NULL_HANDLE;
            VkCommandBuffer earlyCompute   = VK_NULL_HANDLE;
            VkCommandBuffer lateGraphics   = VK_NULL_HANDLE;
            VkCommandBuffer lateCompute    = VK_NULL_HANDLE;
            VkCommandBuffer present        = VK_NULL_HANDLE;

            bool validFrame() const {
                return (
                    predrawCompute && 
                    earlyGraphics  &&
                    earlyCompute   &&
                    lateGraphics   && 
                    lateCompute    && 
                    present
                );
            }
        };

        bool isFrameInProgress() const { return isFrameStarted; }
        
        FrameCommandBuffers getCurrentCommandBuffers() const;

        int getFrameIdx() const {
            assert(isFrameStarted && "Failed to get frame index: Frame not in flight.");
            return currentFrameIdx;
        }

        // Recording of command buffers
        FrameCommandBuffers beginFrame();
        void endFrame();

        // Early Graphics
        //  Important computations which the rest of compute and graphics may rely on
        //  (e.g. z-prepass or gbuffer fill)
        void beginEarlyGraphicsRenderPass(VkCommandBuffer commandBuffer);
        void endEarlyGraphicsRenderPass(VkCommandBuffer commandBuffer);
        VkRenderPass getEarlyGraphicsRenderPass() const { return earlyGraphicsRenderPass; }

        // Early Compute
        //  Compute that late graphics will utilize, that requires geometric information.
        void endEarlyCompute(VkCommandBuffer commandBuffer);

        // Late Graphics
        //  The bulk of traditional graphics rendering (e.g. gbuffer resolve & forward+/OIT)
        void beginLateGraphicsRenderPass(VkCommandBuffer commandBuffer);
        void nextLateGraphicsSubpass(VkCommandBuffer commandBuffer);
        void endLateGraphicsRenderPass(VkCommandBuffer commandBuffer);
        VkRenderPass getLateGraphicsRenderPass() const { return lateGraphicsRenderPass; }

        // Post Compute
        //  Mostly responsible for post processing 
        //  - we can interleave this work with the next frame through a dedicated compute queue
        void endLateCompute(VkCommandBuffer commandBuffer);

        // Final swapchain composite
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
        void changeSwapChainPresentMode(bool vsync);
        bool wasSwapChainRecreated() const { return scRecreatedFlag; }
        void resetScRecreatedFlag() { scRecreatedFlag = false; }

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateRenderObjects();
        void recreateSwapChain();
        void recreateGbuffer();

        void createEarlyGraphicsRenderPass();
        void createLateGraphicsRenderPass();
        void createCompositionRenderPass();
        void createFramebuffers();
        void freeFramebuffers();

        void createSyncObjects();
        void freeSyncObjects();

        bool scRecreatedFlag = false;
        bool gbufferRecreatedFlag = false;

        SumiWindow& sumiWindow;
        SumiDevice& sumiDevice;
        SumiConfig& sumiConfig;

        std::vector<VkSemaphore> predrawComputeFinishedSemaphores;
        std::vector<VkSemaphore> earlyGraphicsFinishedSemaphores;
        std::vector<VkSemaphore> earlyComputeFinishedSemaphores;
        std::vector<VkSemaphore> lateGraphicsFinishedSemaphores;
        std::vector<VkSemaphore> lateComputeFinishedSemaphores;

        // Graphics render passes
        uint32_t currentEarlyGraphicsSubpass = 0;
        VkRenderPass earlyGraphicsRenderPass = VK_NULL_HANDLE;
        uint32_t currentLateGraphicsSubpass = 0;
        VkRenderPass lateGraphicsRenderPass = VK_NULL_HANDLE;

        // Post render pass
        VkRenderPass compositionRenderPass = VK_NULL_HANDLE;

        // Command buffers
        std::vector<VkCommandBuffer> predrawComputeCommandBuffers;
        std::vector<VkCommandBuffer> earlyGraphicsCommandBuffers;
        std::vector<VkCommandBuffer> earlyComputeCommandBuffers;
        std::vector<VkCommandBuffer> lateGraphicsCommandBuffers;
        std::vector<VkCommandBuffer> lateComputeCommandBuffers;
        std::vector<VkCommandBuffer> presentCommandBuffers;

        // Frame buffers
        std::vector<VkFramebuffer> swapchainFramebuffers;
        std::vector<VkFramebuffer> earlyGraphicsFramebuffers;
        std::vector<VkFramebuffer> lateGraphicsFramebuffers;

        // Swap chain
        bool swapChainNeedsRecreate = false;
        bool swapChainUseVsync;
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