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
				VkRenderPass gbufferFillRenderPass, 
				uint32_t gbufferFillSubpassIdx,
				VkRenderPass gbufferResolveRenderPass, 
				uint32_t gbufferResolveSubpassIdx,
				VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~DeferredMeshRenderSys();

			DeferredMeshRenderSys(const DeferredMeshRenderSys&) = delete;
			DeferredMeshRenderSys& operator=(const DeferredMeshRenderSys&) = delete;

			void fillGbuffer(FrameInfo &frameInfo);
			void resolveGbuffer(FrameInfo &frameInfo);

		private:
			//void createGbufferSampler();
			void initResolveDescriptors(SumiGbuffer* gbuffer);
			void createPipelineLayouts(VkDescriptorSetLayout globalDescriptorSetLayout);
			void createPipelines(
				VkRenderPass gbufferFillRenderPass, 
				uint32_t gbufferFillSubpassIdx, 
				VkRenderPass gbufferResolveRenderPass,
				uint32_t gbufferResolveSubpassIdx
			);

			SumiDevice& sumiDevice;

			// Gbuffer Fill Descriptors
			// Keep model descriptor layout handles alive for the lifespan of this rendersystem
			std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout;
			std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout;

			// Gbuffer Pipelines
			// Pipelines used all share the same layout, but are configured differently.
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> pipelines;

			// Resolve Descriptors
			std::unique_ptr<SumiDescriptorPool> resolveDescriptorPool;
			std::unique_ptr<SumiDescriptorSetLayout> resolveDescriptorSetLayout;
			VkDescriptorSet resolveDescriptorSet = VK_NULL_HANDLE;

			// Resolve Pipelines
			VkPipelineLayout resolvePipelineLayout = VK_NULL_HANDLE;
			std::unique_ptr<SumiPipeline> resolvePipeline;
	};

}