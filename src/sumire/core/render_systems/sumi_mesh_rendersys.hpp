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

	class SumiMeshRenderSys {
	public:
		SumiMeshRenderSys(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		);
		~SumiMeshRenderSys();

		SumiMeshRenderSys(const SumiMeshRenderSys&) = delete;
		SumiMeshRenderSys& operator=(const SumiMeshRenderSys&) = delete;

		void renderObjects(FrameInfo &frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
		void createPipeline(VkRenderPass renderPass);

		SumiDevice& sumiDevice;

		std::unique_ptr<SumiPipeline> sumiPipeline;
		VkPipelineLayout pipelineLayout;
	};
}