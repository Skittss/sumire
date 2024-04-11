#include <sumire/core/render_systems/post_processor.hpp>
#include <sumire/core/render_systems/data_structs/post_processor_structs.hpp>

#include <sumire/util/vk_check_success.hpp>
#include <sumire/util/sumire_engine_path.hpp>

#include <cassert>
#include <array>

namespace sumire {

	PostProcessor::PostProcessor(
		SumiDevice& device,
		const std::vector<SumiAttachment*>& colorInAttachments,
		VkRenderPass compositeRenderPass
	) : sumiDevice{ device } {
		initDescriptors(colorInAttachments);
		createPipelineLayouts();
		createPipelines(compositeRenderPass);
	}

	PostProcessor::~PostProcessor() {
		vkDestroyPipelineLayout(sumiDevice.device(), computePipelineLayout, nullptr);
		vkDestroyPipelineLayout(sumiDevice.device(), compositePipelineLayout, nullptr);
	}

	void PostProcessor::tonemap(VkCommandBuffer commandBuffer, uint32_t frameIdx) {
		computePipeline->bind(commandBuffer);

		structs::hdrImagePush push{};
		push.resolution = glm::vec2(
			swapchainMirrorImageResolution.width, swapchainMirrorImageResolution.height);

		vkCmdPushConstants(
			commandBuffer,
			computePipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(structs::hdrImagePush),
			&push
		);

		std::array<VkDescriptorSet, 1> tonemapDescriptors{
			swapchainMirrorImageDescriptorSets[frameIdx]
		};

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipelineLayout,
			0, static_cast<uint32_t>(tonemapDescriptors.size()),
			tonemapDescriptors.data(),
			0, nullptr
		);

		// If we lock the aspect ratio to 16:9 we can remove these ceils and thus avoid extra work.
		vkCmdDispatch(commandBuffer, glm::ceil(push.resolution.x / 16), glm::ceil(push.resolution.y / 16), 1);
	}

	void PostProcessor::compositeFrame(VkCommandBuffer commandBuffer, uint32_t frameIdx) {
		compositePipeline->bind(commandBuffer);

		structs::hdrImagePush push{};
		push.resolution = glm::vec2(
			swapchainMirrorImageResolution.width, swapchainMirrorImageResolution.height);

		vkCmdPushConstants(
			commandBuffer,
			compositePipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(structs::hdrImagePush),
			&push
		);

		std::array<VkDescriptorSet, 1> compositorDescriptors{
			swapchainMirrorImageDescriptorSets[frameIdx]
		};

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			compositePipelineLayout,
			0, static_cast<uint32_t>(compositorDescriptors.size()),
			compositorDescriptors.data(),
			0, nullptr
		);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

	void PostProcessor::initDescriptors(const std::vector<SumiAttachment*>& colorInAttachments) {
		assert(colorInAttachments.size() > 0 && "Post processor was passed an empty attachment array.");

		const uint32_t nImages = static_cast<uint32_t>(colorInAttachments.size());

		swapchainMirrorImageResolution = colorInAttachments[0]->getExtent();

		descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(nImages)
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nImages)
			.build();

		// TODO: Could potentially split this layout for compute / composite pipelines
		//        to remove redundant bind to 1 of the 2 shader stages.
		swapchainMirrorImageDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		swapchainMirrorImageDescriptorSets.resize(nImages);
		for (uint32_t i = 0; i < nImages; i++) {
			VkDescriptorImageInfo swapchainImageStoreDescriptor{};
			swapchainImageStoreDescriptor.sampler = VK_NULL_HANDLE;
			swapchainImageStoreDescriptor.imageView = colorInAttachments[i]->getImageView();
			swapchainImageStoreDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			SumiDescriptorWriter(*swapchainMirrorImageDescriptorSetLayout, *descriptorPool)
				.writeImage(0, &swapchainImageStoreDescriptor)
				.build(swapchainMirrorImageDescriptorSets[i]);
		}
	}

	void PostProcessor::updateDescriptors(const std::vector<SumiAttachment*> colorInAttachments) {
		assert(colorInAttachments.size() > 0 && "Post processor was passed an empty attachment array.");

		const uint32_t nImages = static_cast<uint32_t>(colorInAttachments.size());
		if (nImages != swapchainMirrorImageDescriptorSets.size()) {
			throw std::runtime_error("[Sumire::PostProcessor] Number of input images changed without adjusting descriptor pool size.");
		}

		swapchainMirrorImageResolution = colorInAttachments[0]->getExtent();

		for (uint32_t i = 0; i < nImages; i++) {
			VkDescriptorImageInfo swapchainImageStoreDescriptor{};
			swapchainImageStoreDescriptor.sampler = VK_NULL_HANDLE;
			swapchainImageStoreDescriptor.imageView = colorInAttachments[i]->getImageView();
			swapchainImageStoreDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			SumiDescriptorWriter(*swapchainMirrorImageDescriptorSetLayout, *descriptorPool)
				.writeImage(0, &swapchainImageStoreDescriptor)
				.overwrite(swapchainMirrorImageDescriptorSets[i]);
		}
	}

	void PostProcessor::createPipelineLayouts() {

		// Compute
		VkPushConstantRange computeHdrImagePushRange{};
		computeHdrImagePushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		computeHdrImagePushRange.offset = 0;
		computeHdrImagePushRange.size = sizeof(structs::hdrImagePush);

		std::vector<VkDescriptorSetLayout> computeDescriptorSetLayouts{
			swapchainMirrorImageDescriptorSetLayout->getDescriptorSetLayout()
		};

		std::vector<VkPushConstantRange> computePushConstantRanges{
			computeHdrImagePushRange
		};

		VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
		computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(computeDescriptorSetLayouts.size());
		computePipelineLayoutInfo.pSetLayouts = computeDescriptorSetLayouts.data();
		computePipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(computePushConstantRanges.size());
		computePipelineLayoutInfo.pPushConstantRanges = computePushConstantRanges.data();

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &computePipelineLayoutInfo, nullptr, &computePipelineLayout),
			"[Sumire::PostProcessor] Failed to create compute pipeline layout."
		);


		// Composite
		VkPushConstantRange compositeBaseImagePushRange{};
		compositeBaseImagePushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		compositeBaseImagePushRange.offset = 0;
		compositeBaseImagePushRange.size = sizeof(structs::hdrImagePush);

		std::vector<VkPushConstantRange> compositePushConstantRanges{
			compositeBaseImagePushRange
		};

		std::vector<VkDescriptorSetLayout> compositeDescriptorLayouts{
			swapchainMirrorImageDescriptorSetLayout->getDescriptorSetLayout()
		};

		VkPipelineLayoutCreateInfo compositePipelineLayoutInfo{};
		compositePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		compositePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(compositeDescriptorLayouts.size());
		compositePipelineLayoutInfo.pSetLayouts = compositeDescriptorLayouts.data();
		compositePipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(compositePushConstantRanges.size());
		compositePipelineLayoutInfo.pPushConstantRanges = compositePushConstantRanges.data();

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &compositePipelineLayoutInfo, nullptr, &compositePipelineLayout),
			"[Sumire::PostProcessor] Failed to create compute pipeline layout."
		);

	}

	void PostProcessor::createPipelines(VkRenderPass compositeRenderPass) {
		assert(computePipelineLayout != VK_NULL_HANDLE && compositePipelineLayout != VK_NULL_HANDLE
			&& "Cannot create pipelines when pipeline layout is VK_NULL_HANDLE.");

		computePipeline = std::make_unique<SumiComputePipeline>(
			sumiDevice,
			SUMIRE_ENGINE_PATH("shaders/post/tonemap_gt.comp.spv"),
			computePipelineLayout
		);

		PipelineConfigInfo compositePipelineInfo{};
		SumiPipeline::defaultPipelineConfigInfo(compositePipelineInfo);
		compositePipelineInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		compositePipelineInfo.attributeDescriptions.clear();
		compositePipelineInfo.bindingDescriptions.clear();
		compositePipelineInfo.pipelineLayout = compositePipelineLayout;
		compositePipelineInfo.renderPass = compositeRenderPass;

		compositePipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			SUMIRE_ENGINE_PATH("shaders/post/composite.vert.spv"),
			SUMIRE_ENGINE_PATH("shaders/post/composite.frag.spv"),
			compositePipelineInfo
		);
	}

}