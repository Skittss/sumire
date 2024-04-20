#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/core/graphics_pipeline/sumi_texture.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/materials/sumi_material.hpp>

// Model components
#include <sumire/core/models/vertex.hpp>
#include <sumire/core/models/node.hpp>
#include <sumire/core/models/mesh.hpp>
#include <sumire/core/models/primitive.hpp>
#include <sumire/core/models/skin.hpp>
#include <sumire/core/models/animation.hpp>

#include <sumire/core/flags/sumi_pipeline_state_flags.hpp>
#include <sumire/util/gltf_interpolators.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>
#include <string>

namespace sumire {

	class SumiModel {

	public:

		// Data loader struct for SumiModel constructor.
		struct Data {
			// Scene tree
			std::vector<Node*> nodes{};
			std::vector<std::unique_ptr<Node>> flatNodes{};

			// Temporary holders for Mesh data (Uploaded to GPU on model init).
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
			uint32_t meshCount;
			
			// Mesh Skinning Data
			std::vector<std::unique_ptr<Skin>> skins;

			// Animations
			std::vector<std::unique_ptr<Animation>> animations;

			// Temporary holders for textures (not stored as member variables).
			//  for if access is required in initialization
			std::vector<VkSamplerCreateInfo> samplers;
			std::vector<std::shared_ptr<SumiTexture>> textures;

			// Materials
			std::vector<std::unique_ptr<SumiMaterial>> materials;
			
			~Data() {
				flatNodes.clear();
				nodes.clear();
				skins.clear();
				animations.clear();
				textures.clear();
				materials.clear();
			}
		};

		SumiModel(SumiDevice &device, SumiModel::Data &data);
		~SumiModel();

		SumiModel(const SumiModel&) = delete;
		SumiModel& operator=(const SumiModel&) = delete;

		static std::unique_ptr<SumiDescriptorSetLayout> meshNodeDescriptorLayout(SumiDevice &device);
		static std::unique_ptr<SumiDescriptorSetLayout> matTextureDescriptorLayout(SumiDevice &device);
		static std::unique_ptr<SumiDescriptorSetLayout> matStorageDescriptorLayout(SumiDevice &device);

		uint32_t getAnimationCount() { return static_cast<uint32_t>(animations.size()); }
		bool hasIndices() { return useIndexBuffer; }

		void bind(VkCommandBuffer commandbuffer);
		void draw(
			VkCommandBuffer commandbuffer, 
			VkPipelineLayout pipelineLayout,
			const std::unordered_map<
				SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> &pipelines
		);

		void updateAnimations(const std::vector<uint32_t> indices, float time, bool loop = true);
		void updateAnimation(uint32_t animIdx, float time, bool loop = true);
		void updateNodes();

		std::string displayName{"Unnamed"};

	private:
		void drawNode(
			Node *node, 
			VkCommandBuffer commandBuffer, 
			VkPipelineLayout pipelineLayout,
			const std::unordered_map<
				SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>
			> &pipelines
		);

		// Resource Initializers
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffer(const std::vector<uint32_t> &indices);
		void createDefaultTextures();
		void initDescriptors();
		void createMaterialStorageBuffer();

		SumiDevice &sumiDevice;

		//SumiModel::Data modelData;

		// Model data
		std::vector<Node*> nodes{};
		std::vector<std::unique_ptr<Node>> flatNodes{};

		uint32_t meshCount;

		std::vector<std::unique_ptr<Skin>> skins;
		std::vector<std::unique_ptr<Animation>> animations;

		std::vector<std::unique_ptr<SumiMaterial>> materials;

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
		VkDescriptorSet materialStorageDescriptorSet = VK_NULL_HANDLE;
		// Texture Descriptor sets are stored in material textures,
		//	 and mesh node descriptor sets are stored in SumiModel::Mesh

		// Buffers
		std::unique_ptr<SumiBuffer> materialStorageBuffer; // for static materials

		// Default Textures & Materials
		// TODO: These could be cached
		std::shared_ptr<SumiTexture> emptyTexture;
	};
}