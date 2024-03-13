#pragma once

#include <sumire/core/sumi_pipeline.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core//models/sumi_model.hpp>
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
			void createPipelines(VkRenderPass renderPass);

			SumiDevice& sumiDevice;

			// Keep model descriptor layout handles alive for the lifespan of this rendersystem
			std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout;

			// Pipelines used all share the same layout, but are configured differently.
			VkPipelineLayout pipelineLayout;
			std::unordered_map<SumiMaterial::RequiredPipelineType, std::unique_ptr<SumiPipeline>> pipelines;
		};
}