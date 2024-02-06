#pragma once

#include <sumire/core/sumi_pipeline.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_model.hpp>
#include <sumire/core/sumi_object.hpp>
#include <sumire/core/sumi_camera.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class SumiRenderSystem {
	public:
		SumiRenderSystem(SumiDevice& device, VkRenderPass renderPass);
		~SumiRenderSystem();

		SumiRenderSystem(const SumiRenderSystem&) = delete;
		SumiRenderSystem& operator=(const SumiRenderSystem&) = delete;

		void renderObjects(VkCommandBuffer commandBuffer, std::vector<SumiObject> &objects, const SumiCamera& camera);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		SumiDevice& sumiDevice;

		std::unique_ptr<SumiPipeline> sumiPipeline;
		VkPipelineLayout pipelineLayout;
	};
}