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

	class MeshRenderSys {
		public:
			MeshRenderSys(
				SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~MeshRenderSys();

			MeshRenderSys(const MeshRenderSys&) = delete;
			MeshRenderSys& operator=(const MeshRenderSys&) = delete;

			void renderObjects(FrameInfo &frameInfo);

		private:
			void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
			void createPipeline(VkRenderPass renderPass);

			SumiDevice& sumiDevice;

			// Keep model descriptor layout handles alive for the lifespan of this rendersystem
			std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout;

			std::unique_ptr<SumiPipeline> sumiPipeline;
			VkPipelineLayout pipelineLayout;
		};
}