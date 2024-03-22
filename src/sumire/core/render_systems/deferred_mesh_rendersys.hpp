#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/sumi_object.hpp>
#include <sumire/core/rendering/sumi_camera.hpp>
#include <sumire/core/rendering/sumi_frame_info.hpp>
#include <sumire/core/rendering/sumi_gbuffer.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class DeferredMeshRenderSys {
		public:
			DeferredMeshRenderSys(
				SumiDevice& device, 
				SumiGbuffer* gbuffer,
				VkRenderPass gbufferRenderPass, 
				VkRenderPass compositeRenderPass, 
				VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~DeferredMeshRenderSys();

			DeferredMeshRenderSys(const DeferredMeshRenderSys&) = delete;
			DeferredMeshRenderSys& operator=(const DeferredMeshRenderSys&) = delete;

			void renderGbuffer(FrameInfo &frameInfo);
			void renderObjects(FrameInfo &frameInfo);

		private:
			void createGbufferSampler();
			void initCompositeDescriptors(SumiGbuffer* gbuffer);
			void createPipelineLayouts(VkDescriptorSetLayout globalDescriptorSetLayout);
			void createPipelines(VkRenderPass gbufferRenderPass, VkRenderPass compositeRenderPass);

			SumiDevice& sumiDevice;

			// Gbuffer Descriptors
			// Keep model descriptor layout handles alive for the lifespan of this rendersystem
			std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout;

			// Gbuffer Pipelines
			// Pipelines used all share the same layout, but are configured differently.
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> pipelines;

			// Composite Descriptors
			std::unique_ptr<SumiDescriptorPool> compositeDescriptorPool;
			std::unique_ptr<SumiDescriptorSetLayout> compositeDescriptorSetLayout;
			VkDescriptorSet compositeDescriptorSet = VK_NULL_HANDLE;
			VkSampler gbufferSampler = VK_NULL_HANDLE;

			// Composite Pipelines
			VkPipelineLayout compositePipelineLayout = VK_NULL_HANDLE;
			std::unique_ptr<SumiPipeline> compositePipeline;
	};

}