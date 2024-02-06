#pragma once

#include <sumire/core/sumi_device.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class SumiModel {

	public: 

		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		
			bool operator==(const Vertex &other) const {
				return (
					position == other.position && 
					color == other.color &&
					normal == other.normal &&
					uv == other.uv
				);
			}
		};

		struct Data {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string &filepath);
		};

		SumiModel(SumiDevice &device, const SumiModel::Data &data);
		~SumiModel();

		SumiModel(const SumiModel&) = delete;
		SumiModel& operator=(const SumiModel&) = delete;

		static std::unique_ptr<SumiModel> createFromFile(SumiDevice &device, const std::string &filepath);
		void loadModel(const std::string &filepath, SumiModel::Data &data);

		void bind(VkCommandBuffer commandbuffer);
		void draw(VkCommandBuffer commandbuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffer(const std::vector<uint32_t> &indices);

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