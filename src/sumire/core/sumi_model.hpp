#pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>
#include <sumire/core/materials/sumi_material.hpp>
#include <sumire/core/sumi_texture.hpp>
#include <sumire/core/sumi_descriptors.hpp>

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

		struct Primitive {
			uint32_t firstIndex;
			uint32_t indexCount;
			uint32_t vertexCount;
			SumiMaterial *material{nullptr};

			Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, SumiMaterial *material)
				: firstIndex{firstIndex}, indexCount{indexCount}, vertexCount{vertexCount},
				material{material} {}
		};

		struct Mesh {
			std::vector<std::shared_ptr<Primitive>> primitives;

			struct UniformData {
				glm::mat4 matrix;
			} uniforms;
			
			// Unform Buffer & Descriptor Set
			std::unique_ptr<SumiBuffer> uniformBuffer; // Only one buffer as constant between swap-chain images
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			Mesh(SumiDevice &device, glm::mat4 matrix);

			~Mesh() {
				primitives.clear();
				uniformBuffer = nullptr;
			}
		};

		// TODO: Node is significant enough to maybe warrant its own sub-class
		//         to hold static members for its descriptor layout, etc.
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
			uint32_t meshCount;

			// Textures
			std::vector<VkSamplerCreateInfo> samplers;
			std::vector<std::shared_ptr<SumiTexture>> textures;

			// Materials
			std::vector<std::shared_ptr<SumiMaterial>> materials;
			
			~Data() {
				nodes.clear();
				flatNodes.clear();
				textures.clear();
				materials.clear();
			}
		};

		SumiModel(SumiDevice &device, SumiModel::Data data);
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
		void createDefaultTextures();
		void initDescriptors();

		// Loading Entry point
		static void loadModel(SumiDevice &device, const std::string &filepath, SumiModel::Data &data);

		// .obj loading
		static void loadOBJ(SumiDevice &device, const std::string &filepath, SumiModel::Data &data);

		// .gltf loading
		static void loadGLTF(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool isBinaryFile);
		static void loadGLTFsamplers(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
		static void loadGLTFtextures(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
		static void loadGLTFmaterials(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
		static void getGLTFnodeProperties(
			const tinygltf::Node &node, const tinygltf::Model &model, 
			uint32_t &vertexCount, uint32_t &indexCount,
			SumiModel::Data &data
		);
		static void loadGLTFnode(
			SumiDevice &device, 
			std::shared_ptr<Node> parent, const tinygltf::Node &node, uint32_t nodeIdx, 
			const tinygltf::Model &model, 
			SumiModel::Data &data
		);

		SumiModel::Data modelData;

		// Vertex Buffer params
		std::unique_ptr<SumiBuffer> vertexBuffer;
		uint32_t vertexCount;

		// Index Buffer params
		bool useIndexBuffer = true;
		std::unique_ptr<SumiBuffer> indexBuffer;
		uint32_t indexCount;

		// Descriptors
		std::unique_ptr<SumiDescriptorPool> meshNodeDescriptorPool;
		std::unique_ptr<SumiDescriptorPool> materialDescriptorPool;
		std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorSetLayout;
		std::vector<VkDescriptorSet> meshNodeDescriptorSets;
		//std::vector<VkDescriptorSet> materialDescriptorSets;

		// Buffers
		std::vector<std::unique_ptr<SumiBuffer>> meshNodeUniformBuffers;
		std::vector<std::unique_ptr<SumiBuffer>> materialUniformBuffers;
		std::unique_ptr<SumiBuffer> materialBuffer;

		// Default Textures & Materials
		// TODO: These could be cached
		std::shared_ptr<SumiTexture> emptyTexture;

		SumiDevice &sumiDevice;
	};
}