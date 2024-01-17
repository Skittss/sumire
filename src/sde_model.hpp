#pragma once

#include "sde_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace sde {

	class SdeModel {

	public: 

		struct Vertex {
			glm::vec2 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		SdeModel(SdeDevice& device, const std::vector<Vertex>& vertices);
		~SdeModel();

		SdeModel(const SdeModel&) = delete;
		SdeModel& operator=(const SdeModel&) = delete;

		void bind(VkCommandBuffer commandbuffer);
		void draw(VkCommandBuffer commandbuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);

		SdeDevice& sdeDevice;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}