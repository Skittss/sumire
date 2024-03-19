#include <sumire/core/render_systems/grid_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/grid_rendersys_structs.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <sumire/core/rendering/sumi_swap_chain.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

	GridRendersys::GridRendersys(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		) : sumiDevice{device} {
		createGridQuadBuffers(); // Note: SumiBuffer cleans itself up.
		createPipelineLayout(globalDescriptorSetLayout);
		createPipeline(renderPass);
	}

	GridRendersys::~GridRendersys() {
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
		gridUniformBuffers.clear();
		gridDescriptorPool = nullptr;
		quadVertexBuffer = nullptr;
		quadIndexBuffer = nullptr;
	}

	// Creates vertex and index buffers for a {-1.0, -1.0} -> {1.0, 1.0} XZ quad.
	void GridRendersys::createGridQuadBuffers() {

		GridMinimalVertex vertices[4] = {
			{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}, // pos, uv
			{{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
			{{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
			{{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
		};
		uint32_t indices[6] = {0, 1, 2, 1, 2, 3};

		// Vertex Buffer - Could use with abstracting this code.
		uint32_t vertexInstanceSize = sizeof(GridMinimalVertex);
		VkDeviceSize vertBufferSize = vertexInstanceSize * 4; // vb size

		SumiBuffer vertStagingBuffer{
			sumiDevice,
			vertexInstanceSize,
			4,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		vertStagingBuffer.map();
		vertStagingBuffer.writeToBuffer((void *)vertices);

		quadVertexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			vertexInstanceSize,
			4,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(vertStagingBuffer.getBuffer(), quadVertexBuffer->getBuffer(), vertBufferSize);

		// Index Buffer
		uint32_t indexInstanceSize = sizeof(indices[0]);
		VkDeviceSize indexBufferSize = indexInstanceSize * 6; // ib size
		
		SumiBuffer indexStagingBuffer{
			sumiDevice,
			indexInstanceSize,
			6,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		indexStagingBuffer.map();
		indexStagingBuffer.writeToBuffer((void *)indices);

		quadIndexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			indexInstanceSize,
			6,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(indexStagingBuffer.getBuffer(), quadIndexBuffer->getBuffer(), indexBufferSize);
	}

	void GridRendersys::createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout) {

		// Push Constants
        VkPushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		vertPushConstantRange.offset = 0;
		vertPushConstantRange.size = sizeof(structs::GridVertPushConstantData);

		VkPushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragPushConstantRange.offset = sizeof(structs::GridVertPushConstantData);
		fragPushConstantRange.size = sizeof(structs::GridFragPushConstantData);

		std::vector<VkPushConstantRange> pushConstantRanges{
			vertPushConstantRange,
			fragPushConstantRange
		};

		// Grid Uniform Descriptor Pool
		gridDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

		// Grid Uniform Buffers
		gridUniformBuffers = std::vector<std::unique_ptr<SumiBuffer>>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {

			gridUniformBuffers[i] = std::make_unique<SumiBuffer>(
				sumiDevice,
				sizeof(GridUBOdata),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			gridUniformBuffers[i]->map();
		}

		// Grid Descriptor Set Layout
		auto gridDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		// Grid Descriptor Set
		gridDescriptorSets = std::vector<VkDescriptorSet>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < gridDescriptorSets.size(); i++) {
			auto gridBufferInfo = gridUniformBuffers[i]->descriptorInfo();
			SumiDescriptorWriter(*gridDescriptorSetLayout, *gridDescriptorPool)
				.writeBuffer(0, &gridBufferInfo)
				.build(gridDescriptorSets[i]);
		}
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
			globalDescriptorSetLayout,
			gridDescriptorSetLayout->getDescriptorSetLayout()
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
			"[Sumire::GridRenderSys] Failed to create grid rendering pipeline layout."
		);
	}

	std::vector<VkVertexInputAttributeDescription> GridRendersys::GridMinimalVertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({0, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(GridMinimalVertex, pos)});
		attributeDescriptions.push_back({1, 0 , VK_FORMAT_R32G32_SFLOAT,    offsetof(GridMinimalVertex, uv)});

		return attributeDescriptions;
	}

	std::vector<VkVertexInputBindingDescription> GridRendersys::GridMinimalVertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(GridMinimalVertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	void GridRendersys::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "<GridRenderSys>: Cannot create pipeline before pipeline layout.");

		PipelineConfigInfo pipelineConfig{};
		SumiPipeline::defaultPipelineConfigInfo(pipelineConfig);
		SumiPipeline::enableAlphaBlending(pipelineConfig); // use alpha blending
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // no culling as it messes with the fs quad.
		pipelineConfig.attributeDescriptions = GridMinimalVertex::getAttributeDescriptions();
		pipelineConfig.bindingDescriptions = GridMinimalVertex::getBindingDescriptions();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		sumiPipeline = std::make_unique<SumiPipeline>(
			sumiDevice,
			"shaders/grid.vert.spv",
			"shaders/grid.frag.spv",
			pipelineConfig);

	}

	void GridRendersys::bindGridQuadBuffers(VkCommandBuffer &commandBuffer) {
		VkBuffer vertBuffers[] = { quadVertexBuffer->getBuffer() };
		VkDeviceSize vertOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertBuffers, vertOffsets);
		vkCmdBindIndexBuffer(commandBuffer, quadIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

	void GridRendersys::render(FrameInfo &frameInfo, GridRendersys::GridUBOdata &uniforms) {
		sumiPipeline->bind(frameInfo.commandBuffer);

		std::vector<VkDescriptorSet> frameDescriptorSets{
			frameInfo.globalDescriptorSet,
			gridDescriptorSets[frameInfo.frameIdx]
		};

		// descriptors
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 2,
			frameDescriptorSets.data(),
			0, nullptr
		);

		gridUniformBuffers[frameInfo.frameIdx]->writeToBuffer(&uniforms);
		gridUniformBuffers[frameInfo.frameIdx]->flush();

		// push constants
		structs::GridVertPushConstantData vertPush{};
		vertPush.modelMatrix = glm::mat4{1.0f}; // identity

		vkCmdPushConstants(
			frameInfo.commandBuffer, 
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(structs::GridVertPushConstantData),
			&vertPush
		);

		structs::GridFragPushConstantData fragPush{};
		fragPush.cameraPos = frameInfo.camera.transform.getTranslation();
		fragPush.majorLineThickness = 0.02f;

		vkCmdPushConstants(
			frameInfo.commandBuffer, 
			pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(structs::GridVertPushConstantData),
			sizeof(structs::GridFragPushConstantData),
			&fragPush
		);

		// bind & draw grid quad
		bindGridQuadBuffers(frameInfo.commandBuffer);
		vkCmdDrawIndexed(frameInfo.commandBuffer, 6, 1, 0, 0, 0);
	}
}