#pragma once

#include <sumire/core/sumi_window.hpp>
#include <sumire/core/sumi_swap_chain.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_model.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class SumiRenderer {
	public:
		SumiRenderer(SumiWindow& window, SumiDevice& device);
		~SumiRenderer();

		SumiRenderer(const SumiRenderer&) = delete;
		SumiRenderer& operator=(const SumiRenderer&) = delete;

        VkRenderPass getSwapChainRenderPass() const { return sumiSwapChain->getRenderPass(); }
        float getAspect() const { return sumiSwapChain->extentAspectRatio(); }
        bool isFrameInProgress() const { return isFrameStarted; }
        
        VkCommandBuffer getCurrentCommandBuffer() const { 
            assert(isFrameStarted && "Failed to get command buffer from a frame not in flight.");
            return commandBuffers[currentFrameIdx];
        }

        int getFrameIdx() const {
            assert(isFrameStarted && "Failed to get frame index (frame not in flight).");
            return currentFrameIdx;
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		SumiWindow& sumiWindow;
		SumiDevice& sumiDevice;
		std::unique_ptr<SumiSwapChain> sumiSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIdx;
        int currentFrameIdx{0};
        bool isFrameStarted{false};
	};
}