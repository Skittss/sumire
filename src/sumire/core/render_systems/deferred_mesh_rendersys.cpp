#include <sumire/core/render_systems/deferred_mesh_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/deferred_mesh_rendersys_structs.hpp>

#include <sumire/util/sumire_engine_path.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <sumire/core/flags/sumi_pipeline_state_flags.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	DeferredMeshRenderSys::DeferredMeshRenderSys(
		SumiDevice& device,
		SumiGbuffer* gbuffer,
		VkRenderPass gbufferFillRenderPass,
		uint32_t gbufferFillSubpassIdx,
		VkRenderPass gbufferResolveRenderPass,
		uint32_t gbufferResolveSubpassIdx,
		VkDescriptorSetLayout globalDescriptorSetLayout
	) : sumiDevice{ device } {
		
		// Check physical device can support the size of push constants for this pipeline.
		//  VK min guarantee is 128 bytes, this pipeline targets 256 bytes.
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &deviceProperties);
		uint32_t requiredPushConstantSize = static_cast<uint32_t>(
			sizeof(structs::VertPushConstantData) + sizeof(structs::FragPushConstantData
		));
		if (deviceProperties.limits.maxPushConstantsSize < requiredPushConstantSize) {
			throw std::runtime_error(
				"Mesh rendering requires at least" + 
				std::to_string(requiredPushConstantSize) +
				" bytes of push constant storage. Physical device used supports only " +
				std::to_string(deviceProperties.limits.maxPushConstantsSize) +
				" bytes."
			);
		}

		//createGbufferSampler();
		initResolveDescriptors(gbuffer);
		createPipelineLayouts(globalDescriptorSetLayout);
		createPipelines(
			gbufferFillRenderPass, gbufferFillSubpassIdx, 
			gbufferResolveRenderPass, gbufferResolveSubpassIdx
		);
	}

	DeferredMeshRenderSys::~DeferredMeshRenderSys() {
		vkDestroyPipelineLayout(sumiDevice.device(), resolvePipelineLayout, nullptr);
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	void DeferredMeshRenderSys::initResolveDescriptors(SumiGbuffer* gbuffer) {
		resolveDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(4)
			.addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4)
			.build();

		resolveDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		// Gbuffer attachment descriptors
		VkDescriptorImageInfo positionDescriptor{};
		positionDescriptor.sampler = VK_NULL_HANDLE;
		positionDescriptor.imageView = gbuffer->positionAttachment()->getImageView();
		positionDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo normalDescriptor{};
		normalDescriptor.sampler = VK_NULL_HANDLE;
		normalDescriptor.imageView = gbuffer->normalAttachment()->getImageView();
		normalDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo albedoDescriptor{};
		albedoDescriptor.sampler = VK_NULL_HANDLE;
		albedoDescriptor.imageView = gbuffer->albedoAttachment()->getImageView();
		albedoDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo aoMetalRoughEmissive{};
		aoMetalRoughEmissive.sampler = VK_NULL_HANDLE;
		aoMetalRoughEmissive.imageView = gbuffer->aoMetalRoughEmissiveAttachment()->getImageView();
		aoMetalRoughEmissive.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		SumiDescriptorWriter(*resolveDescriptorSetLayout, *resolveDescriptorPool)
			.writeImage(0, &positionDescriptor)
			.writeImage(1, &normalDescriptor)
			.writeImage(2, &albedoDescriptor)
			.writeImage(3, &aoMetalRoughEmissive)
			.build(resolveDescriptorSet);
	}

	void DeferredMeshRenderSys::createPipelineLayouts(VkDescriptorSetLayout globalDescriptorSetLayout) {
		// Gbuffer Pipelines (Mesh rendering)
		VkPushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		vertPushConstantRange.offset = 0;
		vertPushConstantRange.size = sizeof(structs::VertPushConstantData);

		VkPushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragPushConstantRange.offset = sizeof(structs::VertPushConstantData);
		fragPushConstantRange.size = sizeof(structs::FragPushConstantData);

		std::vector<VkPushConstantRange> pushConstantRanges{
			vertPushConstantRange,
			fragPushConstantRange
		};

		matStorageDescriptorLayout = SumiModel::matStorageDescriptorLayout(sumiDevice);
		meshNodeDescriptorLayout = SumiModel::meshNodeDescriptorLayout(sumiDevice);
		matTextureDescriptorLayout = SumiModel::matTextureDescriptorLayout(sumiDevice);
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
			globalDescriptorSetLayout,
			matTextureDescriptorLayout->getDescriptorSetLayout(),
			meshNodeDescriptorLayout->getDescriptorSetLayout(),
			matStorageDescriptorLayout->getDescriptorSetLayout()
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
			"[Sumire::DeferredMeshRenderSys] Failed to create gbuffer rendering pipeline layout."
		);

		VkPushConstantRange resolvePushConstantRange{};
		resolvePushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		resolvePushConstantRange.offset = 0;
		resolvePushConstantRange.size = sizeof(structs::CompositePushConstantData);

		// Composite Pipeline (Lighting)
		std::vector<VkDescriptorSetLayout> resolveDescriptorSetLayouts{
			globalDescriptorSetLayout,
			resolveDescriptorSetLayout->getDescriptorSetLayout()
		};

		VkPipelineLayoutCreateInfo resolvePipelineLayoutInfo{};
		resolvePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		resolvePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(resolveDescriptorSetLayouts.size());
		resolvePipelineLayoutInfo.pSetLayouts = resolveDescriptorSetLayouts.data();
		resolvePipelineLayoutInfo.pushConstantRangeCount = 1;
		resolvePipelineLayoutInfo.pPushConstantRanges = &resolvePushConstantRange;

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &resolvePipelineLayoutInfo, nullptr, &resolvePipelineLayout),
			"[Sumire::DeferredMeshRenderSys] Failed to create lighting composition pipeline layout."
		);
	}

	void DeferredMeshRenderSys::createPipelines(
		VkRenderPass fillRenderPass,
		uint32_t fillSubpassIdx,
		VkRenderPass resolveRenderPass,
		uint32_t resolveSubpassIdx
	) {
		assert(pipelineLayout != VK_NULL_HANDLE && resolvePipelineLayout != VK_NULL_HANDLE
			&& "[Sumire::DeferredMeshRenderSys]: Cannot create pipelines before pipeline layouts.");

		// TODO: This pipeline creation step should be moved to a common location so that it can be
		//		 re-used by different render systems. This would also allow for efficient caching
		//		 of these pipelines (i.e. do not recreate if it already exists.)
		// TODO: For now, we can generate all permutations of required pipelines during initialization
		//		 so long as the number of flags remains small. If/when it gets larger, we should generate the
		//		 required pipelines at runtime, with (runtime) caching, and generate a common (serialized) 
		//		 pipeline cache that can load up frequently used pipelines from disk.

		// Gbuffer Pipeline - 3 Color attachments

		// Default pipeline config used as base of permutations
		PipelineConfigInfo defaultConfig{};
		SumiPipeline::defaultPipelineConfigInfo(defaultConfig);
		defaultConfig.renderPass = fillRenderPass;
		defaultConfig.subpass = fillSubpassIdx;
		defaultConfig.pipelineLayout = pipelineLayout;

		// Modify color blending for 5 channels as we have 5 colour outputs for deferred rendering.
		//  Validation errors will occur without this. (even though colour blending is not used for the attachments)
		std::array<VkPipelineColorBlendAttachmentState, 5> colorBlendAttachments;
		colorBlendAttachments.fill(defaultConfig.colorBlendAttachment);
		defaultConfig.colorBlendInfo.pAttachments = colorBlendAttachments.data();
		defaultConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());

		std::string defaultVertShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill.vert.spv");
		std::string defaultFragShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill.frag.spv");

		// All permutations (including default)
		const uint32_t pipelinePermutations = 2 * SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_HIGHEST;
		for (uint32_t i = 0; i < pipelinePermutations; i++) {
			SumiPipelineStateFlags permutationFlags = static_cast<SumiPipelineStateFlags>(i);
			PipelineConfigInfo permutationConfig = defaultConfig;
			std::string permutationVertShader = defaultVertShader;
			std::string permutationFragShader = defaultFragShader;

			// Deal with each bit flag
			if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_UNLIT_BIT) {
				permutationVertShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill_unlit.vert.spv");
				permutationFragShader = SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_fill_unlit.frag.spv");
			}
			if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_DOUBLE_SIDED_BIT)
				permutationConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

			// Create pipeline and map it
			std::unique_ptr<SumiPipeline> permutationPipeline = std::make_unique<SumiPipeline>(
				sumiDevice,
				permutationVertShader, 
				permutationFragShader,
				permutationConfig
			);
			pipelines.emplace(permutationFlags, std::move(permutationPipeline));
		}

		// Resolve pipeline
		PipelineConfigInfo resolvePipelineConfig{};
		SumiPipeline::defaultPipelineConfigInfo(resolvePipelineConfig);
		SumiPipeline::enableAlphaBlending(resolvePipelineConfig);
		resolvePipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		resolvePipelineConfig.attributeDescriptions.clear();
		resolvePipelineConfig.bindingDescriptions.clear();
		resolvePipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		resolvePipelineConfig.renderPass = resolveRenderPass;
		resolvePipelineConfig.subpass = resolveSubpassIdx;
		resolvePipelineConfig.pipelineLayout = resolvePipelineLayout;

		resolvePipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_resolve.vert.spv"),
			SUMIRE_ENGINE_PATH("shaders/deferred/mesh_gbuffer_resolve.frag.spv"),
			resolvePipelineConfig
		);
	}

	void DeferredMeshRenderSys::resolveGbuffer(FrameInfo &frameInfo) {

		// Bind composite pipeline
		resolvePipeline->bind(frameInfo.commandBuffer);

		structs::CompositePushConstantData push{};
		push.nLights = 1;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			resolvePipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(structs::CompositePushConstantData),
			&push
		);

		std::array<VkDescriptorSet, 2> resolveDescriptors{
			frameInfo.globalDescriptorSet,
			resolveDescriptorSet
		};

		// Bind descriptor sets (Gbuffer)
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			resolvePipelineLayout,
			0, static_cast<uint32_t>(resolveDescriptors.size()),
			resolveDescriptors.data(),
			0, nullptr
		);

		// Composite with a FS quad
		vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
	}

	void DeferredMeshRenderSys::fillGbuffer(FrameInfo &frameInfo) {

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&frameInfo.globalDescriptorSet,
			0, nullptr
		);

		for (auto& kv: frameInfo.objects) {
			auto& obj = kv.second;
			
			// Only render objects with a mesh
			if (obj.model == nullptr) continue;

			structs::VertPushConstantData push{};
			push.modelMatrix = obj.transform.modelMatrix();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer, 
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(structs::VertPushConstantData),
				&push
			);

			// TODO: Link animation playback (e.g. index, timer, loop) to UI.
			//		 For now, play all animations, looped.
			// TODO: This update can and should be done on a separate thread.
			//		 Updating joint matrices may need to be made thread safe / double buffered as a result.
			if (obj.model->getAnimationCount() > 0) {
				std::vector<uint32_t> indices(obj.model->getAnimationCount());
				for (uint32_t i = 0; i < indices.size(); i++) indices[i] = i;
				obj.model->updateAnimations(indices, frameInfo.cumulativeFrameTime);
			}
			
			// SumiModel handles the binding of descriptor sets 1-3 and frag push constants
			obj.model->bind(frameInfo.commandBuffer);
			// Each draw command may need a different pipeline, so the model draw binds pipelines at call time.
			obj.model->draw(frameInfo.commandBuffer, pipelineLayout, pipelines);
		}
	}
}