#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/sumi_object.hpp>
#include <sumire/core/rendering/sumi_camera.hpp>
#include <sumire/core/rendering/sumi_frame_info.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class DeferredMeshRenderSys {
		public:
			DeferredMeshRenderSys(
				SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~DeferredMeshRenderSys();

			DeferredMeshRenderSys(const DeferredMeshRenderSys&) = delete;
			DeferredMeshRenderSys& operator=(const DeferredMeshRenderSys&) = delete;

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
			std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> pipelines;
		};
}