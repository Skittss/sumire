#pragma once

#include <sumire/core/sumi_pipeline.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_model.hpp>
#include <sumire/core/sumi_object.hpp>
#include <sumire/core/sumi_camera.hpp>
#include <sumire/core/sumi_frame_info.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class SumiRenderSystem {
	public:
		SumiRenderSystem(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		);
		~SumiRenderSystem();

		SumiRenderSystem(const SumiRenderSystem&) = delete;
		SumiRenderSystem& operator=(const SumiRenderSystem&) = delete;

		void renderObjects(FrameInfo &frameInfo, std::vector<SumiObject> &objects);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
		void createPipeline(VkRenderPass renderPass);

		SumiDevice& sumiDevice;

		std::unique_ptr<SumiPipeline> sumiPipeline;
		VkPipelineLayout pipelineLayout;
	};
}