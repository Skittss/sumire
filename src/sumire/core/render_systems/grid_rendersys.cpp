#include <sumire/core/render_systems/grid_rendersys.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include "grid_rendersys.hpp"

namespace sumire {

	struct GridVertPushConstantData {
		alignas(16) glm::mat4 modelMatrix;
	};

	struct GridFragPushConstantData {
		alignas(8) glm::vec2 gridOrigin;
		alignas(4) float majorLineThickness;
	};

	GridRendersys::GridRendersys(
			SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
		) : sumiDevice{device} {
		createGridQuadBuffers(); // Note: SumiBuffer cleans itself up.
		createPipelineLayout(globalDescriptorSetLayout);
		createPipeline(renderPass);
	}

	GridRendersys::~GridRendersys() {
		// TODO: Destroy pipeline at this line??
		vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
	}

	// Creates vertex and index buffers for a {-0.5, 0.5} -> {0.5 -> 0.5} XZ quad.
	void GridRendersys::createGridQuadBuffers() {

		// XZ quad from {-0.5, 0.5} -> {0.5 -> 0.5}
		GridMinimalVertex vertices[4] = {
			{{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f}}, // pos, uv
			{{ 0.5f, 0.0f, -0.5f}, {1.0f, 0.0f}},
			{{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f}},
			{{ 0.5f, 0.0f,  0.5f}, {1.0f, 1.0f}},
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

        VkPushConstantRange vertPushConstantRange{};
		vertPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		vertPushConstantRange.offset = 0;
		vertPushConstantRange.size = sizeof(GridVertPushConstantData);

		VkPushConstantRange fragPushConstantRange{};
		fragPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragPushConstantRange.offset = sizeof(GridVertPushConstantData);
		fragPushConstantRange.size = sizeof(GridFragPushConstantData);

		std::vector<VkPushConstantRange> pushConstantRanges{
			vertPushConstantRange,
			fragPushConstantRange
		};
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		if (vkCreatePipelineLayout(
				sumiDevice.device(), 
				&pipelineLayoutInfo,
				nullptr, 
				&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("<GridRenderSys>: Failed to create pipeline layout.");
		}
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

	void GridRendersys::render(FrameInfo &frameInfo) {
		sumiPipeline->bind(frameInfo.commandBuffer);

		// descriptors
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&frameInfo.globalDescriptorSet,
			0, nullptr
		);

		// push constants
		GridVertPushConstantData vertPush{};
		vertPush.modelMatrix = glm::mat4{1.0f}; // identity

		vkCmdPushConstants(
			frameInfo.commandBuffer, 
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(GridVertPushConstantData),
			&vertPush
		);

		GridFragPushConstantData fragPush{};
		fragPush.gridOrigin = {0.0f, 0.0f};
		fragPush.majorLineThickness = 0.02f;

		vkCmdPushConstants(
			frameInfo.commandBuffer, 
			pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(GridVertPushConstantData),
			sizeof(GridFragPushConstantData),
			&fragPush
		);

		// bind & draw grid quad
		bindGridQuadBuffers(frameInfo.commandBuffer);
		vkCmdDrawIndexed(frameInfo.commandBuffer, 6, 1, 0, 0, 0);

		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);

	}
}