#pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>
#include <sumire/core/materials/sumi_material.hpp>
#include <sumire/core/sumi_texture.hpp>
#include <sumire/core/sumi_descriptors.hpp>

// Model components
#include <sumire/core/models/vertex.hpp>

#include <sumire/util/gltf_interpolators.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>
#include <string>

// TODO: Pass this in to shaders as a specialization constant
#define MODEL_MAX_JOINTS 256u

namespace sumire {

	class SumiModel {

	public:

		struct Node;

		struct Primitive {
			uint32_t firstIndex;
			uint32_t indexCount;
			uint32_t vertexCount;
			SumiMaterial *material{nullptr};
			uint32_t materialIdx;

			Primitive(
				uint32_t firstIndex, 
				uint32_t indexCount, uint32_t vertexCount, 
				SumiMaterial *material, uint32_t materialIdx
			) : firstIndex{firstIndex}, indexCount{indexCount}, vertexCount{vertexCount},
				material{material}, materialIdx{materialIdx} {}
		};

		struct Mesh {
			std::vector<std::unique_ptr<Primitive>> primitives;

			struct UniformData {
				glm::mat4 matrix;
				// glm::mat4 jointMatrices[MODEL_MAX_JOINTS] {};
				// glm::mat4 jointNormalMatrices[MODEL_MAX_JOINTS] {};
				int nJoints{ 0 };
			} uniforms;

			struct JointData {
				glm::mat4 jointMatrix;
				glm::mat4 jointNormalMatrix;
			};
			
			// Unform Buffer & Descriptor Set
			// Only one buffer as constant between swap-chain images
			std::unique_ptr<SumiBuffer> uniformBuffer = VK_NULL_HANDLE; 
			// for skinning & animation, stored in host-coherent memory not local gpu memory.
			std::unique_ptr<SumiBuffer> jointBuffer = VK_NULL_HANDLE; 
			// Note: This descriptor is PARTIALLY_BOUND as jointBuffer can be VK_NULL_HANDLE if there is no skin.
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			Mesh(SumiDevice &device, glm::mat4 matrix);
			~Mesh() {
				primitives.clear();
				uniformBuffer = nullptr;
			}

			void initJointBuffer(SumiDevice &device, uint32_t nJoints);
		};

		struct Skin {
			std::string name;
			Node *skeletonRoot{nullptr};
			std::vector<glm::mat4> inverseBindMatrices;
			std::vector<Node*> joints;

			~Skin() {
				skeletonRoot = nullptr;
				joints.clear();
			}
		};

		// TODO: Node is significant enough to maybe warrant its own sub-class
		//         to hold static members for its descriptor layout, etc.
		struct Node {
			uint32_t idx;
			Node *parent;
			std::vector<Node*> children;
			std::string name;
			std::unique_ptr<Mesh> mesh; // Optional

			// Node transform properties
			glm::mat4 matrix{ 1.0f };
			glm::vec3 translation{ 0.0f };
			glm::quat rotation;
			glm::vec3 scale{ 1.0f };

			glm::mat4 cachedLocalTransform{ 1.0f };
			glm::mat4 worldTransform{ 1.0f };
			glm::mat4 invWorldTransform{ 1.0f };
			glm::mat4 normalMatrix{ 1.0f };

			void setMatrix(glm::mat4 matrix);
			void setTranslation(glm::vec3 translation);
			void setRotation(glm::quat rotation);
			void setScale(glm::vec3 scale);

			glm::mat4 getLocalTransform();
			glm::mat4 getGlobalTransform();

			// Skinning (Optional)
			Skin *skin;
			int32_t skinIdx{-1};

			// Update matrices, skinning, and joints
			void applyTransformHierarchy();
			void updateRecursive();
			void update();
			bool needsUpdate = true;

			~Node() {
				mesh = nullptr;
				parent = nullptr;
				skin = nullptr;
				children.clear();
			}
		};

		struct AnimationChannel {
			enum PathType { TRANSLATION, ROTATION, SCALE, WEIGHTS };
			PathType path;
			Node *node;
			uint32_t samplerIdx;
			
			~AnimationChannel() {
				node = nullptr;
			};
		};

		struct AnimationSampler {
			util::GLTFinterpolationType interpolation;
			std::vector<float> inputs;
			std::vector<glm::vec4> outputs;
		};

		struct Animation {
			std::string name;
			std::vector<AnimationSampler> samplers;
			std::vector<AnimationChannel> channels;
			float start = std::numeric_limits<float>::max();
			float end = std::numeric_limits<float>::min();

			~Animation() {
				channels.clear();
			}
		};

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
		void draw(VkCommandBuffer commandbuffer, VkPipelineLayout pipelineLayout);

		void updateAnimations(const std::vector<uint32_t> indices, float time, bool loop = true);
		void updateAnimation(uint32_t animIdx, float time, bool loop = true);
		void updateNodes();

		std::string displayName{"Unnamed"};

	private:
		void drawNode(Node *node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

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