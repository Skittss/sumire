#pragma once

#include "sumi_window.hpp"
#include "sumi_pipeline.hpp"
#include "sumi_swap_chain.hpp"
#include "sumi_device.hpp"
#include "sumi_model.hpp"
#include "sumi_object.hpp"

#include <memory>
#include <vector>

namespace sumire {

	class SumiRenderer {
	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		void run();

		SumiRenderer();
		~SumiRenderer();

		SumiRenderer(const SumiRenderer&) = delete;
		SumiRenderer& operator=(const SumiRenderer&) = delete;

	private:
		void loadObjects();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();
		void recordCommandBuffer(int imageIdx);
		void renderObjects(VkCommandBuffer commandBuffer);

		SumiWindow sumiWindow{ WIDTH, HEIGHT, "Sumire" };
		SumiDevice sumiDevice{ sumiWindow };
		std::unique_ptr<SumiSwapChain> sumiSwapChain;
		std::unique_ptr<SumiPipeline> sumiPipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<SumiObject> objects;
	};
}