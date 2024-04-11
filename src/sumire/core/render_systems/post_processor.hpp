#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

#include <memory>

namespace sumire {

	class PostProcessor {
	public:

		PostProcessor(
			SumiDevice& device,
			const std::vector<SumiAttachment*>& colorInAttachments,
			VkRenderPass compositeRenderPass
		);
		~PostProcessor();

		PostProcessor(const PostProcessor&) = delete;
		PostProcessor& operator=(const PostProcessor&) = delete;

		void updateDescriptors(const std::vector<SumiAttachment*> colorInAttachments);

		enum TonemapCurve {
			LINEAR,
			GT,
			ACES_FILM
		};

		void tonemap(VkCommandBuffer commandBuffer, uint32_t frameIdx);
		void bloom(VkCommandBuffer commandBuffer);
		void compositeFrame(VkCommandBuffer commandBuffer, uint32_t frameIdx);

	private:
		void initDescriptors(const std::vector<SumiAttachment*>& colorInAttachments);
		void createPipelineLayouts();
		void createPipelines(VkRenderPass compositeRenderPass);

		SumiDevice& sumiDevice;

		std::unique_ptr<SumiDescriptorPool> descriptorPool;
		
		// Descriptor sets for swapchain images as input / output.
		VkExtent2D swapchainMirrorImageResolution;
		std::unique_ptr<SumiDescriptorSetLayout> swapchainMirrorImageDescriptorSetLayout;
		std::vector<VkDescriptorSet> swapchainMirrorImageDescriptorSets;

		std::unique_ptr<SumiComputePipeline> computePipeline;
		VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;

		std::unique_ptr<SumiPipeline> compositePipeline;
		VkPipelineLayout compositePipelineLayout = VK_NULL_HANDLE;

	};

}