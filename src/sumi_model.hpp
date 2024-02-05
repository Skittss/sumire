#pragma once

#include "sumi_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace sumire {

	class SumiModel {

	public: 

		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		struct Data {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};

		SumiModel(SumiDevice& device, const SumiModel::Data &data);
		~SumiModel();

		SumiModel(const SumiModel&) = delete;
		SumiModel& operator=(const SumiModel&) = delete;

		void bind(VkCommandBuffer commandbuffer);
		void draw(VkCommandBuffer commandbuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);

		SumiDevice& sumiDevice;

		// Vertex Buffer params
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;

		// Index Buffer params
		bool useIndexBuffer = true;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;
	};
}