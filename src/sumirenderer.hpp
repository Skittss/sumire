#pragma once

#include "sumi_window.hpp"
#include "sumi_pipeline.hpp"
#include "sumi_swap_chain.hpp"
#include "sumi_device.hpp"
#include "sumi_model.hpp"

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
		void loadModels();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();
		void recordCommandBuffer(int imageIdx);

		SumiWindow sdeWindow{ WIDTH, HEIGHT, "Sumire" };
		SumiDevice sdeDevice{ sdeWindow };
		std::unique_ptr<SumiSwapChain> sdeSwapChain;
		std::unique_ptr<SumiPipeline> sdePipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<SumiModel> sdeModel;
	};
}