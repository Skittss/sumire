#include <sumire/core/render_systems/deferred_mesh_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/deferred_mesh_rendersys_structs.hpp>

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
		
		//vkDestroySampler(sumiDevice.device(), gbufferSampler, nullptr);
	}

	//void DeferredMeshRenderSys::createGbufferSampler() {
	//	VkSamplerCreateInfo samplerCreateInfo{};
	//	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	//	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	//	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	//	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	//	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	//	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	//	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	//	samplerCreateInfo.maxAnisotropy = 1.0f;
	//	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	//	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	//	samplerCreateInfo.compareEnable = VK_FALSE;
	//	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	//	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	//	samplerCreateInfo.mipLodBias = 0.0f;
	//	samplerCreateInfo.minLod = 0.0f;
	//	samplerCreateInfo.maxLod = 1.0f;

	//	VK_CHECK_SUCCESS(
	//		vkCreateSampler(sumiDevice.device(), &samplerCreateInfo, nullptr, &gbufferSampler),
	//		"[Sumire::DeferredMeshRenderSys] Failed to create gbuffer sampler."
	//	);
	//}

	void DeferredMeshRenderSys::initResolveDescriptors(SumiGbuffer* gbuffer) {
		resolveDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(3)
			.addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3)
			// .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.build();

		resolveDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
			// .addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
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

		SumiDescriptorWriter(*resolveDescriptorSetLayout, *resolveDescriptorPool)
			.writeImage(0, &positionDescriptor)
			.writeImage(1, &normalDescriptor)
			.writeImage(2, &albedoDescriptor)
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

		VkPushConstantRange compositePushConstantRange{};
		compositePushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		compositePushConstantRange.offset = 0;
		compositePushConstantRange.size = sizeof(structs::CompositePushConstantData);

		// Composite Pipeline (Lighting)
		std::vector<VkDescriptorSetLayout> compositeDescriptorSetLayouts{
			resolveDescriptorSetLayout->getDescriptorSetLayout()
		};

		VkPipelineLayoutCreateInfo compositePipelineLayoutInfo{};
		compositePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		compositePipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(compositeDescriptorSetLayouts.size());
		compositePipelineLayoutInfo.pSetLayouts = compositeDescriptorSetLayouts.data();
		compositePipelineLayoutInfo.pushConstantRangeCount = 1;
		compositePipelineLayoutInfo.pPushConstantRanges = &compositePushConstantRange;

		VK_CHECK_SUCCESS(
			vkCreatePipelineLayout(
				sumiDevice.device(), &compositePipelineLayoutInfo, nullptr, &resolvePipelineLayout),
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

		// Modify color blending for 3 channels as we have 3 colour outputs for deferred rendering.
		//  Validation errors will occur without this. (even though colour blending is not used for the attachments)
		std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;
		colorBlendAttachments.fill(defaultConfig.colorBlendAttachment);
		defaultConfig.colorBlendInfo.pAttachments = colorBlendAttachments.data();
		defaultConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());

		std::string defaultVertShader = "shaders/deferred/mesh_gbuffer_fill.vert.spv";
		std::string defaultFragShader = "shaders/deferred/mesh_gbuffer_fill.frag.spv";

		// All permutations (including default)
		const uint32_t pipelinePermutations = 2 * SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_HIGHEST;
		for (uint32_t i = 0; i < pipelinePermutations; i++) {
			SumiPipelineStateFlags permutationFlags = static_cast<SumiPipelineStateFlags>(i);
			PipelineConfigInfo permutationConfig = defaultConfig;
			std::string permutationVertShader = defaultVertShader;
			std::string permutationFragShader = defaultFragShader;

			// Deal with each bit flag
			// TODO: We cannot handle unlit rendering in this way.
			// 		 We should render these objects directly to the out color buffer to bypass the lighting
			// 		 system entirely.
			// if (permutationFlags & SumiPipelineStateFlagBits::SUMI_PIPELINE_STATE_UNLIT_BIT)
			// 	permutationFragShader = "shaders/forward/mesh_unlit.frag.spv";
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
			"shaders/deferred/mesh_gbuffer_resolve.vert.spv",
			"shaders/deferred/mesh_gbuffer_resolve.frag.spv",
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

		// Bind descriptor sets (Gbuffer & lights)
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			resolvePipelineLayout,
			0, 1,
			&resolveDescriptorSet,
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