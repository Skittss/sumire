#pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <tiny_gltf.h>

#include <memory>
#include <vector>
#include <string>

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
		};

		SumiModel(SumiDevice &device, const SumiModel::Data &data);
		~SumiModel();

		SumiModel(const SumiModel&) = delete;
		SumiModel& operator=(const SumiModel&) = delete;

		static std::unique_ptr<SumiModel> createFromFile(SumiDevice &device, const std::string &filepath);

		void bind(VkCommandBuffer commandbuffer);
		void draw(VkCommandBuffer commandbuffer);

		std::string displayName{"Unnamed"};

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffer(const std::vector<uint32_t> &indices);

		static void loadModel(const std::string &filepath, SumiModel::Data &data);

		// TODO: Loading may need separate classes for models in the future.
		// OBJ
		static void loadOBJ(const std::string &filepath, SumiModel::Data &data);

		// .gltf
		static void loadGLTF(const std::string &filepath, SumiModel::Data &data);
		static void getGLTFnodeProperties(
			const tinygltf::Node &node, const tinygltf::Model &model, uint32_t &vertexCount, uint32_t &indexCount
		);
		static void loadGLTFnode(const tinygltf::Node &node, uint32_t nodeIdx, const tinygltf::Model &model);

		SumiDevice &sumiDevice;

		// Vertex Buffer params

		std::unique_ptr<SumiBuffer> vertexBuffer;
		uint32_t vertexCount;

		// Index Buffer params
		bool useIndexBuffer = true;
		std::unique_ptr<SumiBuffer> indexBuffer;
		uint32_t indexCount;
	};
}