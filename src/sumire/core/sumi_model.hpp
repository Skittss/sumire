#pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <tiny_gltf.h>

#include <memory>
#include <vector>
#include <string>

namespace sumire {

	class SumiModel {

	public:

		struct Texture {
			uint32_t width;
			uint32_t height;
		};

		struct Primitive {
			uint32_t firstIndex;
			uint32_t indexCount;
			uint32_t vertexCount;

			Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount)
				: firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount) {}
		};

		struct Mesh {
			std::vector<std::shared_ptr<Primitive>> primitives;

			~Mesh() {
				primitives.clear();
			}
		};

		struct Node {
			uint32_t idx;
			std::shared_ptr<Node> parent;
			std::vector<std::shared_ptr<Node>> children;
			std::string name;
			std::shared_ptr<Mesh> mesh;

			// Node transform properties
			glm::mat4 matrix;
			glm::vec3 translation{};
			glm::quat rotation{};
			glm::vec3 scale{1.0f};

			glm::mat4 getLocalTransform();
			glm::mat4 getGlobalTransform();

			~Node() {
				parent = nullptr;
				children.clear();
				mesh = nullptr;
			}
		};

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

			// Scene tree
			std::vector<std::shared_ptr<Node>> nodes{};
			// TODO: This flat list is useful for gltf only?
			std::vector<std::shared_ptr<Node>> flatNodes{};

			// Mesh data
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			// Textures
			std::vector<std::shared_ptr<Texture>> textures;
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
		void drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer);

		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffer(const std::vector<uint32_t> &indices);

		// Loading Entry point
		static void loadModel(const std::string &filepath, SumiModel::Data &data);

		// .obj loading
		static void loadOBJ(const std::string &filepath, SumiModel::Data &data);

		// .gltf loading
		static void loadGLTF(const std::string &filepath, SumiModel::Data &data, bool isBinaryFile);
		static void loadGLTFtextures(tinygltf::Model &model, SumiModel::Data &data);
		static void getGLTFnodeProperties(
			const tinygltf::Node &node, const tinygltf::Model &model, uint32_t &vertexCount, uint32_t &indexCount
		);
		static void loadGLTFnode(std::shared_ptr<Node> parent, const tinygltf::Node &node, uint32_t nodeIdx, const tinygltf::Model &model, SumiModel::Data &data);

		// Mesh tree
		std::vector<std::shared_ptr<Node>> nodes;
		std::vector<std::shared_ptr<Node>> flatNodes;

		// Meshes
		// Vertex Buffer params
		std::unique_ptr<SumiBuffer> vertexBuffer;
		uint32_t vertexCount;

		// Index Buffer params
		bool useIndexBuffer = true;
		std::unique_ptr<SumiBuffer> indexBuffer;
		uint32_t indexCount;

		SumiDevice &sumiDevice;
	};
}