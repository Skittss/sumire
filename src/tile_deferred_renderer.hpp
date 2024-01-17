#pragma once

#include "sde_window.hpp"
#include "sde_pipeline.hpp"
#include "sde_swap_chain.hpp"
#include "sde_device.hpp"
#include "sde_model.hpp"

#include <memory>
#include <vector>

namespace sde {

	class TileDeferredRenderer {
	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		void run();

		TileDeferredRenderer();
		~TileDeferredRenderer();

		TileDeferredRenderer(const TileDeferredRenderer&) = delete;
		TileDeferredRenderer& operator=(const TileDeferredRenderer&) = delete;

	private:
		void loadModels();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();
		void recordCommandBuffer(int imageIdx);

		SdeWindow sdeWindow{ WIDTH, HEIGHT, "TDR" };
		SdeDevice sdeDevice{ sdeWindow };
		std::unique_ptr<SdeSwapChain> sdeSwapChain;
		std::unique_ptr<SdePipeline> sdePipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<SdeModel> sdeModel;
	};
}