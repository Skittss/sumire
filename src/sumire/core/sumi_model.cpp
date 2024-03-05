#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION // No exceptions (for now?)

#include <sumire/core/sumi_model.hpp>
#include <sumire/core/render_systems/data_structs/mesh_rendersys_structs.hpp>

#include <sumire/core/sumi_swap_chain.hpp>
#include <sumire/util/gltf_vulkan_flag_converters.hpp>
#include <sumire/util/gltf_interpolators.hpp>
#include <sumire/math/math_utils.hpp>

// TODO: Could we find a way around using experimental GLM hashing? (though it seems stable)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <unordered_map>

// Deal with cmake building to ./build instead of ./
#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace std {
	template <>
	struct hash<sumire::SumiModel::Vertex> {
		size_t operator()(sumire::SumiModel::Vertex const &vertex) const {
			size_t seed = 0;
			sumire::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace sumire {

	// TODO: Can the data copy here be optimised to not copy the large materials and texture arrays?
	SumiModel::SumiModel(SumiDevice &device, SumiModel::Data data) 
		: sumiDevice{ device }, modelData{ data }
	{
		createVertexBuffers(data.vertices);
		createIndexBuffer(data.indices);
		createDefaultTextures();
		initDescriptors();
		createMaterialStorageBuffer();
	}

	SumiModel::~SumiModel() {
		materialDescriptorPool = nullptr;
		meshNodeDescriptorPool = nullptr;
		indexBuffer = nullptr;
		vertexBuffer = nullptr;
	}

	std::unique_ptr<SumiModel> SumiModel::createFromFile(SumiDevice &device, const std::string &filepath) {
		std::filesystem::path fp = filepath;
		Data data{};
		loadModel(device, filepath, data);

		auto modelPtr = std::make_unique<SumiModel>(device, data);
		modelPtr->displayName = fp.filename().u8string();

		std::cout << "Loaded Model <" << filepath << "> (verts: " << data.vertices.size() 
				  << ", nodes: " << data.flatNodes.size() 
				  << ", mat: " << data.materials.size()
				  << ", tex: " << data.textures.size()
				  << ")" << std::endl;
		return modelPtr;
	}

	void SumiModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		uint32_t vertexInstanceSize = sizeof(vertices[0]);
		VkDeviceSize bufferSize = vertexInstanceSize * vertexCount; // vb size

		SumiBuffer stagingBuffer{
			sumiDevice,
			vertexInstanceSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)vertices.data());

		vertexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			vertexInstanceSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void SumiModel::createIndexBuffer(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		useIndexBuffer = indexCount > 0;

		if (!useIndexBuffer) return; 
		
		uint32_t indexInstanceSize = sizeof(indices[0]);
		VkDeviceSize bufferSize = indexInstanceSize * indexCount; // ib size
		
		SumiBuffer stagingBuffer{
			sumiDevice,
			indexInstanceSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)indices.data());

		indexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			indexInstanceSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void SumiModel::createDefaultTextures() {
		// Empty texture
		VkImageCreateInfo imageInfo{};
		SumiTexture::defaultImageCreateInfo(imageInfo);
		VkSamplerCreateInfo samplerInfo{};
		SumiTexture::defaultSamplerCreateInfo(sumiDevice, samplerInfo);

		// TODO: Make this texture compressed (KTX / DDS) if possible
		emptyTexture = SumiTexture::createFromFile(
			sumiDevice,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			imageInfo,
			samplerInfo,
			"../assets/textures/empty.png"
		);
	}

	void SumiModel::initDescriptors() {

		// == Mesh Nodes ====================================================================
		// - To push local transform matrices to the shader via UBO

		// Per-Node descriptor pool
		meshNodeDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(
				modelData.meshCount * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Local matrices
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				modelData.meshCount * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

		// Nodes descriptor set layout
		auto meshNodeDescriptorSetLayout = SumiModel::meshNodeDescriptorLayout(sumiDevice);

		// Per-Node Descriptor Sets for local matrices
		//   Iterate flat nodes to skip doing recursion here on children.
		for (auto &node : modelData.flatNodes) {
			if (node->mesh) {
				auto bufferInfo = node->mesh->uniformBuffer->descriptorInfo();
				SumiDescriptorWriter(*meshNodeDescriptorSetLayout, *meshNodeDescriptorPool)
					.writeBuffer(0, &bufferInfo)
					.build(node->mesh->descriptorSet);
			}
		}

		// == Materials =====================================================================
		// - To push textures and material factors to the shader via CIS and UBO

		// TODO: Offload this descriptor pool to a material manager class for the whole scene.
		materialDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(
				(1 + modelData.materials.size() * SumiMaterial::MAT_TEX_COUNT) * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Texture samplers
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				modelData.materials.size() * SumiMaterial::MAT_TEX_COUNT * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Single SSBO for materials
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
			.build();

		// Read in Descriptor Set Layout from material class
		auto materialDescriptorLayout = SumiMaterial::getDescriptorSetLayout(sumiDevice);

		// Per-Material Descriptor Sets for Texture Samplers
		for (auto& mat : modelData.materials) {
			// Write to images
			mat->writeDescriptorSet(*materialDescriptorPool, *materialDescriptorLayout, emptyTexture.get());
		}
	}

	void SumiModel::createMaterialStorageBuffer() {

		// Gather material info
		std::vector<SumiMaterial::MaterialShaderData> matShaderData;
		for (auto& mat : modelData.materials) {
			matShaderData.push_back(mat->getMaterialShaderData());
		}

		VkDeviceSize bufferSize = matShaderData.size() * sizeof(SumiMaterial::MaterialShaderData);

		// Create Storage Buffer
		materialStorageBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		// Descriptor set layout
		auto matStorageDescriptorLayout = SumiModel::matStorageDescriptorLayout(sumiDevice);

		// Material Storage Buffer Descriptor
		auto bufferInfo = materialStorageBuffer->descriptorInfo();
		SumiDescriptorWriter(*matStorageDescriptorLayout, *materialDescriptorPool)
			.writeBuffer(0, &bufferInfo)
			.build(materialStorageDescriptorSet);

		// Stage and write to device local memory
		SumiBuffer stagingBuffer{
			sumiDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)matShaderData.data());

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), materialStorageBuffer->getBuffer(), bufferSize);
	}

	std::unique_ptr<SumiMaterial> SumiModel::createDefaultMaterial(SumiDevice &device) {
		SumiMaterial::MaterialTextureData matData{};
		// TODO: Set default properties here?
		return SumiMaterial::createMaterial(device, matData);
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::meshNodeDescriptorLayout(SumiDevice &device) {
		return SumiDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::matTextureDescriptorLayout(SumiDevice &device) {
		return SumiMaterial::getDescriptorSetLayout(device);
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::matStorageDescriptorLayout(SumiDevice &device) {
		return SumiDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}

	void SumiModel::bind(VkCommandBuffer commandBuffer) {
		// Vertex and Index Buffers
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (useIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void SumiModel::drawNode(Node *node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
		// Draw this node's primitives
		if (node->mesh) {
			for (auto& primitive : node->mesh->primitives) {

				// Bind descriptor sets
				const std::vector<VkDescriptorSet> descriptorSets{
					primitive->material->getDescriptorSet(),
					node->mesh->descriptorSet,
					materialStorageDescriptorSet,
				};

				// Fragment Shader Push constants (for material indexing)
				structs::FragPushConstantData push{};
				push.materialIdx = primitive->materialIdx;

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					sizeof(structs::VertPushConstantData),
					sizeof(structs::FragPushConstantData),
					&push
				);

				// IMPORTANT: Reserve set 0 for global descriptors
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout,
					1,
					static_cast<uint32_t>(descriptorSets.size()),
					descriptorSets.data(),
					0, nullptr
				);
				
				// Draw
				if (primitive->indexCount > 0) {
					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				} else {
					vkCmdDraw(commandBuffer, primitive->vertexCount, 1, 0, 0);
				}
			}
		}

		// Draw children
		for (auto& child : node->children) {
			drawNode(child, commandBuffer, pipelineLayout);
		}
	}

	// Draw a model node tree. *Starts binding descriptors from set 1*
	void SumiModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
		for (auto& node : modelData.nodes) {
			drawNode(node.get(), commandBuffer, pipelineLayout);
		}
	}

	// Update a range of animations for this model.
	void SumiModel::updateAnimations(const std::vector<uint32_t> indices, float time, bool loop) {
		if (modelData.animations.empty() || indices.empty()) return;

		for (const uint32_t &i : indices) {
			updateAnimation(i, time, loop);
		}
	}

	void SumiModel::updateAnimation(uint32_t animIdx, float time, bool loop) {
		assert(animIdx < modelData.animations.size() && animIdx >= 0 && "Animation index out of range");
		
		std::shared_ptr<Animation> animation = modelData.animations[animIdx];

		// loop animation
		// TODO: I'm not sure this works if time < animation->start
		//		 it would be better to use math::fmod here
		if (loop && time > animation->end) {
			float duration = animation->end - animation->start;
			float exceeds = time - animation->start - duration;
			float n_times_exceeds = std::floor(exceeds/duration);
			time = animation->start + (exceeds - n_times_exceeds * duration);
		}

		// TODO: Perhaps this "changed" flag should be given to each node so only updated nodes are affected.
		bool modelUpdated = false;
		for (auto& channel : animation->channels) {
			AnimationSampler &sampler = animation->samplers[channel.samplerIdx];
			
			// Note: Only loop to penultimate animation state as we interpolate between keyframes
			//		 i and i + 1.
			for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
				
				// current keyframe (0) and next keyframe (1) input data
				float input0 = sampler.inputs[i];
				float input1 = sampler.inputs[i + 1];

				// Only update this animation when we are within its specified time-frame
				if (time < input0 || time > input1) continue;

				// Interpolation value
				float u = std::max(0.0f, time - input0) / (input1 - input0);
				if (u < 0.0f || u > 1.0f) continue;

				// current keyframe (0) and next keyframe (1) output data
				glm::vec4 output0;
				glm::vec4 output1;
				// (output data to fill if cubic spline):
				glm::vec4 inTangent0;
				glm::vec4 outTangent0;
				glm::vec4 inTangent1;
				glm::vec4 outTangent1;

				// Fill output data
				switch (sampler.interpolation) {
					case util::INTERP_STEP:
					case util::INTERP_LINEAR: {
						output0 = sampler.outputs[i];
						output1 = sampler.outputs[i + 1];
					}
					break;
					case util::INTERP_CUBIC_SPLINE: {
						// Cublic spline data is formatted as (in tangent, value, out tangent) triplets.
						float offset0 = 3*i;
						float offset1 = 3*(i+1);
						inTangent0  = sampler.outputs[offset0];
						output0     = sampler.outputs[offset0 + 1];
						outTangent0 = sampler.outputs[offset0 + 2];
						inTangent1  = sampler.outputs[offset1];
						output1     = sampler.outputs[offset1 + 1];
						outTangent1 = sampler.outputs[offset1 + 2];
					}
					break;
				}

				// Update pointed mesh node parameters (T,S,R,W)
				switch (channel.path) {
					case AnimationChannel::PathType::TRANSLATION: {
						glm::vec4 translation = util::interpVec4(output0, outTangent0, output1, inTangent1, u, sampler.interpolation);;
						channel.node->translation = glm::vec3(translation);
					}
					break;
					case AnimationChannel::PathType::SCALE: {
						glm::vec4 scale = util::interpVec4(output0, outTangent0, output1, inTangent1, u, sampler.interpolation);
						channel.node->scale = glm::vec3(scale);
					}
					break;
					case AnimationChannel::PathType::ROTATION: {
						glm::quat q0;
						q0.x = output0.x; 
						q0.y = output0.y; 
						q0.z = output0.z; 
						q0.w = output0.w;
						glm::quat q0_ot = glm::quat{outTangent0.x, outTangent0.y, outTangent0.z, outTangent0.w};
						q0_ot.x = outTangent0.x; 
						q0_ot.y = outTangent0.y;
						q0_ot.x = outTangent0.z; 
						q0_ot.y = outTangent0.w;
						glm::quat q1;
						q1.x = output1.x; 
						q1.y = output1.y; 
						q1.z = output1.z; 
						q1.w = output1.w;
						glm::quat q1_it = glm::quat(inTangent1.x, inTangent1.y, inTangent1.z, inTangent1.w);
						q1_it.x = inTangent1.x; 
						q1_it.y = inTangent1.y; 
						q1_it.z = inTangent1.z; 
						q1_it.w = inTangent1.w;

						channel.node->rotation = util::interpQuat(q0, q0_ot, q1, q1_it, u, sampler.interpolation);
					}
					break;
					case AnimationChannel::PathType::WEIGHTS: {
						std::runtime_error("TODO: Morph target animation update via weights.");
					}
					break;
				}

				modelUpdated = true;
			}
		}

		// Only update nodes if we advanced any animation channels
		if (modelUpdated) {
			updateNodes();
		}
	}

	void SumiModel::updateNodes() {
		for (auto& node : modelData.nodes) {
			node->update();
		}
	}

	void SumiModel::loadModel(SumiDevice &device, const std::string &filepath, SumiModel::Data &data) {
		std::filesystem::path fp = filepath;
		std::filesystem::path ext = fp.extension();

		if (ext == ".obj") 
			loadOBJ(device, filepath, data);
		else if (ext == ".gltf") 
			loadGLTF(device, filepath, data, false);
		else if (ext == ".glb")
			loadGLTF(device, filepath, data, true);
		else
			throw std::runtime_error("Attempted to load unsupported model type: <" + ext.u8string() + ">");

	}

	void SumiModel::loadOBJ(SumiDevice &device, const std::string &filepath, SumiModel::Data &data) {
		// .Obj loading
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			throw std::runtime_error(warn + err);
		}

		data.vertices.clear();
		data.indices.clear();

		std::shared_ptr<Node> mainNode = std::make_shared<Node>();
		mainNode->idx = 0;
		mainNode->parent = nullptr;
		mainNode->name = "mesh";
		mainNode->mesh = std::make_shared<Mesh>(device, glm::mat4{1.0});

		std::unordered_map<Vertex, uint32_t> uniqueVertices{}; // map for calculating index buffer from obj

		for (const auto &shape : shapes) {
			for (const auto &index : shape.mesh.indices) {
				Vertex vertex{};

				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0], 
						attrib.vertices[3 * index.vertex_index + 1], 
						attrib.vertices[3 * index.vertex_index + 2]
					};

					// Colour support
					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0], 
						attrib.colors[3 * index.vertex_index + 1], 
						attrib.colors[3 * index.vertex_index + 2]
					};
				} 

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0], 
						attrib.normals[3 * index.normal_index + 1], 
						attrib.normals[3 * index.normal_index + 2]
					};
				} 

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0], 
						attrib.texcoords[2 * index.texcoord_index + 1], 
					};
				} 

				// Update index buffer map
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(data.vertices.size());
					data.vertices.push_back(vertex);
				}
				data.indices.push_back(uniqueVertices[vertex]);
			}
		}

		// Push default material
		data.materials.push_back(SumiModel::createDefaultMaterial(device));

		std::shared_ptr<Primitive> mainPrimitive = std::make_shared<Primitive>(
			0, 
			data.indices.size(), 
			data.vertices.size(), 
			data.materials.back().get(), 
			0 // Use default material only
		);
		mainNode->mesh->primitives.push_back(std::move(mainPrimitive));
		data.meshCount = 1;

		data.nodes.push_back(mainNode);
		data.flatNodes.push_back(mainNode);
	}

	void SumiModel::loadGLTF(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool isBinaryFile) {
		tinygltf::Model gltfModel;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		// Load file in using tinygltf
		bool loadSuccess = false;
		switch (isBinaryFile) {
			// Load GLB (Binary)
			case true: {
				loadSuccess = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath.c_str());
			}
			break;
			// Load GLTF (Ascii)
			case false: {
				loadSuccess = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath.c_str());
			}
			break;
		}

		if (!loadSuccess) {
			throw std::runtime_error(warn + err);
		}

		// Clear model data struct
		data.nodes.clear();
		data.flatNodes.clear();
		
		data.vertices.clear();
		data.indices.clear();

		// default gltf scene
		// TODO: For now only the default scene is loaded. It would be better to load all scenes or be able
		//		 to specify a scene index.
		if (gltfModel.defaultScene < 0)
			std::cerr << "WARN: Model <" << filepath << "> has no default scene - here be dragons!" << std::endl;

		const int default_scene_idx = std::max(gltfModel.defaultScene, 0);
		const tinygltf::Scene &scene = gltfModel.scenes[default_scene_idx];

		// Textures & Materials
		loadGLTFsamplers(device, gltfModel, data);
		loadGLTFtextures(device, gltfModel, data);
		loadGLTFmaterials(device, gltfModel, data);

		// Mesh information
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		for (uint32_t i = 0; i < scene.nodes.size(); i++) {
			getGLTFnodeProperties(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount, data);
		}

		// Load nodes
		for (uint32_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadGLTFnode(device, nullptr, node, scene.nodes[i], gltfModel, data);
		}

		// Animations
		if (gltfModel.animations.size() > 0) {
			loadGLTFanimations(gltfModel, data);
		}

		// Skinning Information
		// Note: Uses loaded nodes as joints, so ensure to always load nodes first.
		loadGLTFskins(gltfModel, data);

		for (auto& node : data.flatNodes) {
			// Assign skin from loaded skinIdx;
			if(node->skinIdx > -1) {
				node->skin = data.skins[node->skinIdx].get();
			}
			// Update once for initial pose
			if(node->mesh) {
				node->update();
			}
		}
	}

	void SumiModel::loadGLTFsamplers(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {
		
		VkSamplerCreateInfo defaultSamplerInfo{};
		SumiTexture::defaultSamplerCreateInfo(device, defaultSamplerInfo);

		for (tinygltf::Sampler &sampler : model.samplers) {
			VkSamplerCreateInfo samplerInfo = defaultSamplerInfo;
			samplerInfo.minFilter = util::GLTF2VK_FilterMode(sampler.minFilter);
			samplerInfo.magFilter = util::GLTF2VK_FilterMode(sampler.magFilter);
			samplerInfo.addressModeU = util::GLTF2VK_SamplerAddressMode(sampler.wrapS);
			samplerInfo.addressModeV = util::GLTF2VK_SamplerAddressMode(sampler.wrapT);
			samplerInfo.addressModeW = samplerInfo.addressModeV;
			data.samplers.push_back(samplerInfo);
		}
	}

	void SumiModel::loadGLTFtextures(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {

		// Default create infos
		VkImageCreateInfo imageInfo{};
		SumiTexture::defaultImageCreateInfo(imageInfo);
		VkSamplerCreateInfo defaultSamplerInfo{};
		SumiTexture::defaultSamplerCreateInfo(device, defaultSamplerInfo);

		for (tinygltf::Texture &texture : model.textures) {
			tinygltf::Image image = model.images[texture.source];

			VkSamplerCreateInfo samplerInfo{};
			// Sampler selection
			if (texture.sampler > -1) {
				// Use sampler from loaded sampler array
				samplerInfo = data.samplers[texture.sampler];
			} else {
				// Use default
				samplerInfo = defaultSamplerInfo;
			}

			// Texture creation
			std::unique_ptr<SumiTexture> tex;
			if (image.component == 3) {
				// RGB (JPG/PNG) -> RGBA (Vk) texture creation.
				tex = SumiTexture::createFromRGB(
					device,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					imageInfo,
					samplerInfo,
					image.width, image.height,
					image.image.data()
				);
			} else {
				// RGBA (JPG/PNG) -> RGBA (Vk) texture creation.
				tex = SumiTexture::createFromRGBA(
					device,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					imageInfo,
					samplerInfo,
					image.width, image.height,
					image.image.data()
				);
			}

			// TODO: Mip-mapping

			data.textures.push_back(std::move(tex));
		}
	}

	void SumiModel::loadGLTFmaterials(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {
		for (tinygltf::Material &material : model.materials) {
			
			SumiMaterial::MaterialTextureData mat{};

			// Textures
			// TODO: Include texcoord which specify which UV coords from the primitives are used
			//       for the respective texture.
			if (material.pbrMetallicRoughness.baseColorTexture.index > -1) {
				mat.baseColorTexture = data.textures[material.pbrMetallicRoughness.baseColorTexture.index];
				mat.baseColorTexCoord = material.pbrMetallicRoughness.baseColorTexture.texCoord;
			}
			if (material.pbrMetallicRoughness.metallicRoughnessTexture.index > -1) {
				mat.metallicRoughnessTexture = data.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
				mat.metallicRoughnessTexCoord = material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;
			}
			if (material.normalTexture.index > -1) {
				mat.normalTexture = data.textures[material.normalTexture.index];
				mat.normalTexCoord = material.normalTexture.texCoord;
			}
			if (material.emissiveTexture.index > -1) {
				mat.emissiveTexture = data.textures[material.emissiveTexture.index];
				mat.emissiveTexCoord = material.emissiveTexture.texCoord;
			}
			if (material.occlusionTexture.index > -1) {
				mat.occlusionTexture = data.textures[material.occlusionTexture.index];
				mat.occlusionTexCoord = material.occlusionTexture.texCoord;
			}

			// Factors
			mat.baseColorFactors = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
			mat.metallicRoughnessFactors = {
				material.pbrMetallicRoughness.metallicFactor,
				material.pbrMetallicRoughness.roughnessFactor
			};
			mat.emissiveFactors = glm::make_vec3(material.emissiveFactor.data());
			mat.normalScale = material.normalTexture.scale; // TODO: these should be doubles?
			mat.occlusionStrength = material.occlusionTexture.strength;

			// Other properties
			mat.doubleSided = material.doubleSided;
			if (material.alphaMode == "BLEND") {
				mat.alphaMode = SumiMaterial::AlphaMode::MODE_BLEND;
			} else if (material.alphaMode == "MASK") {
				mat.alphaMode = SumiMaterial::AlphaMode::MODE_MASK;
				mat.alphaCutoff = material.alphaCutoff;
			} else {
				mat.alphaMode = SumiMaterial::AlphaMode::MODE_OPAQUE;
			}
			if (!mat.name.empty()) mat.name = material.name;

			data.materials.push_back(SumiMaterial::createMaterial(device, mat));
		}

		// Push Default material to back of material array
		data.materials.push_back(SumiModel::createDefaultMaterial(device));
	}

	void SumiModel::loadGLTFskins(tinygltf::Model &model, SumiModel::Data &data) {
		assert(data.nodes.size() > 0 && "Attempted to load skins before loading model nodes.");
		assert(data.flatNodes.size() > 0 && "Flattened model nodes array was uninitialized when loading skins.");
		
		static int cnt = 0;
		for (tinygltf::Skin &skin : model.skins) {
			std::shared_ptr<Skin> createSkin = std::make_shared<Skin>();
			createSkin->name = skin.name;

			// Skeleton Root Node
			if (skin.skeleton > -1) {
				createSkin->skeletonRoot = getGLTFnode(skin.skeleton, data).get();
			}

			// Joint Nodes
			for (int jointIdx : skin.joints) {
				Node *jointNode = getGLTFnode(jointIdx, data).get();
				if (jointNode != nullptr) {
					createSkin->joints.push_back(jointNode);
				}
			}

			// Inverse Bind Matrices
			if (skin.inverseBindMatrices > -1) {
				const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
				const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer & buffer = model.buffers[bufferView.buffer];

				// Copy and cast raw data into skin's mat4 vector
				createSkin->inverseBindMatrices.resize(accessor.count);
				memcpy(
					createSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], 
					accessor.count * sizeof(glm::mat4)
				);
			}

			data.skins.push_back(std::move(createSkin));
		}
	}

	void SumiModel::loadGLTFanimations(tinygltf::Model &model, SumiModel::Data &data) {
		assert(data.flatNodes.size() > 0 && "Flattened model nodes array was uninitialized when loading animations.");

		for (tinygltf::Animation &animation : model.animations) {
			std::shared_ptr<Animation> createAnimation = std::make_shared<Animation>();
			createAnimation->name = animation.name.empty() ?
				 std::to_string(data.animations.size()) : animation.name;

			// Sampler Information
			for (auto &sampler : animation.samplers) {
				AnimationSampler createSampler{};

				// Interpolation Type
				if (sampler.interpolation == "LINEAR") {
					createSampler.interpolation = util::GLTFinterpolationType::INTERP_LINEAR;
				}
				if (sampler.interpolation == "STEP") {
					createSampler.interpolation = util::GLTFinterpolationType::INTERP_STEP;
				}
				if (sampler.interpolation == "CUBICSPLINE") {
					createSampler.interpolation = util::GLTFinterpolationType::INTERP_CUBIC_SPLINE;
				}

				// Time values (inputs)
				const tinygltf::Accessor &inputAccessor = model.accessors[sampler.input];
				const tinygltf::BufferView &inputBufferView = model.bufferViews[inputAccessor.bufferView];
				const tinygltf::Buffer &inputBuffer = model.buffers[inputBufferView.buffer];

				assert(inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && "Animation sampler input data was not float type");

				const void *inputDataPtr = &inputBuffer.data[inputAccessor.byteOffset + inputBufferView.byteOffset];
				const float *floatData = static_cast<const float*>(inputDataPtr);

				for (size_t idx = 0; idx < inputAccessor.count; idx++) {
					createSampler.inputs.push_back(floatData[idx]);
				}

				// Compile individual sampler start-end values into a single time range for the animation
				for (size_t i = 0; i < createSampler.inputs.size(); i++) {
					float input = createSampler.inputs[i];
					createAnimation->start = std::min(createAnimation->start, input);
					createAnimation->end = std::max(createAnimation->end, input);
				}

				// Translation, Rotation, Scale and Weight values (outputs)
				const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
				const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
				const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

				assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && "Animation sampler output data was not float type");

				const void *outputDataPtr = &outputBuffer.data[outputAccessor.byteOffset + outputBufferView.byteOffset];
				
				switch (outputAccessor.type) {
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3 *vec3Data = static_cast<const glm::vec3*>(outputDataPtr);
						for (size_t idx = 0; idx < outputAccessor.count; idx++) {
							createSampler.outputs.push_back(glm::vec4(vec3Data[idx], 0.0f));
						}
					}
					break;
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4 *vec4Data = static_cast<const glm::vec4*>(outputDataPtr);
						for (size_t idx = 0; idx < outputAccessor.count; idx++) {
							createSampler.outputs.push_back(vec4Data[idx]);
						}
					}
					break;
					default: {
						std::runtime_error("Tried to Cast Unsupported Sampler Output Value Data Type (Supported: Vec3, Vec4)");
					}
				}

				// Check inputs and outputs directly map to one another for interpolation types other than cubic spline.
				if (createSampler.interpolation != util::GLTFinterpolationType::INTERP_CUBIC_SPLINE && 
					createSampler.inputs.size() != createSampler.outputs.size()
				) {
					std::runtime_error("Non cubic spline animation channel has non 1-to-1 mapping of animation inputs (time values) to outputs (morphing values)");
				}

				createAnimation->samplers.push_back(createSampler);
			}
			
			// Animation Channels
			for (auto& channel : animation.channels) {
				AnimationChannel createChannel{};

				if (channel.target_path == "rotation") {
					createChannel.path = AnimationChannel::PathType::ROTATION;
				}
				if (channel.target_path == "translation") {
					createChannel.path = AnimationChannel::PathType::TRANSLATION;
				}
				if (channel.target_path == "scale") {
					createChannel.path = AnimationChannel::PathType::SCALE;
				}
				if (channel.target_path == "weights") {
					std::runtime_error("TODO: Animation via weights & morph targets");
					createChannel.path = AnimationChannel::PathType::WEIGHTS;
				}

				createChannel.samplerIdx = channel.sampler;
				createChannel.node = getGLTFnode(channel.target_node, data).get();
				assert(createChannel.node != nullptr && "Animation reffered to a non-existant model node");

				createAnimation->channels.push_back(createChannel);
			}

			data.animations.push_back(std::move(createAnimation));
		}
	}

	void SumiModel::getGLTFnodeProperties(
		const tinygltf::Node &node, const tinygltf::Model &model, 
		uint32_t &vertexCount, uint32_t &indexCount,
		SumiModel::Data &data
	) {

		// Recursion through node tree
		if (node.children.size() > 0) {
			for (uint32_t i = 0; i < node.children.size(); i++) {
				getGLTFnodeProperties(model.nodes[node.children[i]], model, vertexCount, indexCount, data);
			}
		}

		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (uint32_t i = 0; i < mesh.primitives.size(); i++) {
				auto primitive = mesh.primitives[i];
				vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
				if (primitive.indices > -1) {
					indexCount += model.accessors[primitive.indices].count;
				}
			}
			data.meshCount++;
		}
	}

	void SumiModel::loadGLTFnode(
		SumiDevice &device,
		Node *parent, const tinygltf::Node &node, uint32_t nodeIdx, 
		const tinygltf::Model &model, 
		SumiModel::Data &data
	) {
		std::shared_ptr<Node> createNode = std::make_shared<Node>();
		createNode->idx = nodeIdx;
		createNode->parent = parent;
		createNode->name = node.name;
		createNode->skinIdx = node.skin;
		createNode->matrix =  glm::mat4(1.0f);

		// Local transforms specified by either a 4x4 mat or translation, rotation and scale vectors.
		if (node.matrix.size() == 16) {
			createNode->matrix = glm::make_mat4x4(node.matrix.data());
		}
		if (node.translation.size() == 3) {
			createNode->translation = glm::make_vec3(node.translation.data());
		}
		if (node.rotation.size() == 4) {
			createNode->rotation = glm::make_quat(node.rotation.data());
		}
		if (node.scale.size() == 3) {
			createNode->scale = glm::make_vec3(node.scale.data());
		}

		// Invert y in top-most nodes so that -y is up. (GLTF spec defines +y as up, sumire uses -y = up).
		// if (parent == nullptr) {
		// 	createNode->matrix[2][2] *= -1.0f;
		// }

		// Load node children if exists
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				loadGLTFnode(device, createNode.get(), model.nodes[node.children[i]], node.children[i], model, data);
			}
		}

		// Load mesh if node has it
		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			std::shared_ptr createMesh = std::make_shared<Mesh>(device, createNode->matrix);
			
			for (size_t i = 0; i < mesh.primitives.size(); i++) {

				uint32_t vertexCount = 0;
				uint32_t indexCount = 0;
				uint32_t vertexStart = static_cast<uint32_t>(data.vertices.size());
				uint32_t indexStart = static_cast<uint32_t>(data.indices.size());

				const tinygltf::Primitive &primitive = mesh.primitives[i];
				bool hasIndices = primitive.indices > -1;
				bool hasSkin;

				// Vertices
				{
					// Buffer pointers & data strides
					const float *bufferPos = nullptr;
					const float *bufferNorm = nullptr;
					const float *bufferTexCoord0 = nullptr;
					const float *bufferTexCoord1 = nullptr;
					const float *bufferColor0 = nullptr;
					const void  *rawBufferJoints0 = nullptr;
					const float *bufferWeights0 = nullptr;
					int stridePos;
					int strideNorm;
					int strideTexCoord0;
					int strideTexCoord1;
					int strideColor0;
					int strideJoints0;
					int strideWeights0;

					int jointsComponentType;

					// Pos
					auto positionEntry = primitive.attributes.find("POSITION");
					assert(positionEntry != primitive.attributes.end()); // Must have position

					const tinygltf::Accessor& posAccessor = model.accessors[positionEntry->second];
					vertexCount = static_cast<uint32_t>(posAccessor.count);

					const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
					const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
					bufferPos = reinterpret_cast<const float *>(&(posBuffer.data[posAccessor.byteOffset + posBufferView.byteOffset]));
					stridePos = posAccessor.ByteStride(posBufferView) ? (posAccessor.ByteStride(posBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

					// Normals
					auto normEntry = primitive.attributes.find("NORMAL");
					if (normEntry != primitive.attributes.end()) {
						const tinygltf::Accessor& normAccessor = model.accessors[normEntry->second];
						const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
						const tinygltf::Buffer& normBuffer = model.buffers[normBufferView.buffer];
						bufferNorm = reinterpret_cast<const float *>(&(normBuffer.data[normAccessor.byteOffset + normBufferView.byteOffset]));
						strideNorm = normAccessor.ByteStride(normBufferView) ? (normAccessor.ByteStride(normBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3); 
					}

					// UVs
					// TODO: I think this can be arbitrarily large depending on the materials the model uses?
					//       Query no. of textures first and loop here if this is the case.
					auto texCoordEntry = primitive.attributes.find("TEXCOORD_0");
					if (texCoordEntry != primitive.attributes.end()) {
						const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordEntry->second];
						const tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
						const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordBufferView.buffer];
						bufferTexCoord0 = reinterpret_cast<const float *>(&(texCoordBuffer.data[texCoordAccessor.byteOffset + texCoordBufferView.byteOffset]));
						strideTexCoord0 = texCoordAccessor.ByteStride(texCoordBufferView) ? (texCoordAccessor.ByteStride(texCoordBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2); 
					}

					// Vertex Colours
					auto colorEntry = primitive.attributes.find("COLOR_0");
					if (colorEntry != primitive.attributes.end()) {
						const tinygltf::Accessor& colorAccessor = model.accessors[colorEntry->second];
						const tinygltf::BufferView& colorBufferView = model.bufferViews[colorAccessor.bufferView];
						const tinygltf::Buffer& colorBuffer = model.buffers[colorBufferView.buffer];
						bufferColor0 = reinterpret_cast<const float *>(&(colorBuffer.data[colorAccessor.byteOffset + colorBufferView.byteOffset]));
						strideColor0 = colorAccessor.ByteStride(colorBufferView) ? (colorAccessor.ByteStride(colorBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3); 
					}

					// Skinning
					if (createNode->skinIdx > -1) {
						auto jointsEntry = primitive.attributes.find("JOINTS_0");
						if (jointsEntry != primitive.attributes.end()) {
							const tinygltf::Accessor& jointsAccessor = model.accessors[jointsEntry->second];
							const tinygltf::BufferView& jointsBufferView = model.bufferViews[jointsAccessor.bufferView];
							const tinygltf::Buffer& jointsBuffer = model.buffers[jointsBufferView.buffer];
							jointsComponentType = jointsAccessor.componentType;
							rawBufferJoints0 = &(model.buffers[jointsBufferView.buffer].data[jointsAccessor.byteOffset + jointsBufferView.byteOffset]);
							strideJoints0 = jointsAccessor.ByteStride(jointsBufferView) ? (jointsAccessor.ByteStride(jointsBufferView) / tinygltf::GetComponentSizeInBytes(jointsComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
						}

						auto weightsEntry = primitive.attributes.find("WEIGHTS_0");
						if (weightsEntry != primitive.attributes.end()) {
							const tinygltf::Accessor& weightsAccessor = model.accessors[weightsEntry->second];
							const tinygltf::BufferView& weightsBufferView = model.bufferViews[weightsAccessor.bufferView];
							const tinygltf::Buffer& weightsBuffer = model.buffers[weightsBufferView.buffer];
							bufferWeights0 = reinterpret_cast<const float *>(&(model.buffers[weightsBufferView.buffer].data[weightsAccessor.byteOffset + weightsBufferView.byteOffset]));
							strideWeights0 = weightsAccessor.ByteStride(weightsBufferView) ? (weightsAccessor.ByteStride(weightsBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);

						}
					}

					hasSkin = rawBufferJoints0 && bufferWeights0;

					// Cast skinning buffer to correct type before writing vertices
					const uint16_t *castShortBufferJoints0;
					const uint8_t *castByteBufferJoints0;
					if (hasSkin) {
						switch (jointsComponentType) {
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
								castShortBufferJoints0 = static_cast<const uint16_t*>(rawBufferJoints0);
								break;
							}
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
								castByteBufferJoints0 = static_cast<const uint8_t*>(rawBufferJoints0);
								break;
							}
							default:
								throw std::runtime_error("Attempted to load skin with an unsupported data type. Supported: uint16, uint8");
						}
					}

					// Populate Vertex structs
					for (uint32_t vIdx = 0; vIdx < vertexCount; vIdx++) {
						Vertex v{};
						v.position = glm::make_vec3(&bufferPos[vIdx * stridePos]);
						v.normal = glm::normalize(bufferNorm ? glm::make_vec3(&bufferNorm[vIdx * strideNorm]) : glm::vec3{0.0f});
						v.uv = bufferTexCoord0 ? glm::make_vec2(&bufferTexCoord0[vIdx * strideTexCoord0]) : glm::vec3{0.0f};
						v.color = bufferColor0 ? glm::make_vec3(&bufferColor0[vIdx * strideColor0]) : glm::vec3{1.0f};

						// Skinning information
						if (hasSkin) {
							// Joints
							switch (jointsComponentType) {
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
									v.joint = glm::make_vec4(&castShortBufferJoints0[vIdx * strideJoints0]);
									break;
								}
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
									v.joint = glm::make_vec4(&castByteBufferJoints0[vIdx * strideJoints0]);
									break;
								}
							}

							// Weights
							v.weight = glm::make_vec4(&bufferWeights0[vIdx * strideWeights0]);
							if (glm::length(v.weight) == 0.0f) // disallow zeroed weights
								v.weight = glm::vec4{1.0f, 0.0f, 0.0f, 0.0f};

						} else {
							// Default Joint
							v.joint = glm::vec4{0.0f};

							// Default Weight
							v.weight = glm::vec4{1.0f, 0.0f, 0.0f, 0.0f};
						}

						// TODO: Keeping track of current vertex no. and indexing straight to the array would again be faster than
						//       push_back.
						data.vertices.push_back(v);
					}
				}

				// Indices
				if (hasIndices) {
					const tinygltf::Accessor& idxAccessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
					indexCount = static_cast<uint32_t>(idxAccessor.count);

					const tinygltf::BufferView& idxBufferView = model.bufferViews[idxAccessor.bufferView];
					const tinygltf::Buffer& idxBuffer = model.buffers[idxBufferView.buffer];
					
					// Raw idx data to cast
					const void *idxBufferData = &(idxBuffer.data[idxAccessor.byteOffset + idxBufferView.byteOffset]);

					// TODO: Raw indexing of the indices array would be considerably faster for index buffer
					//       here than push_back.
					switch (idxAccessor.componentType) {
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
							const uint32_t *castData = static_cast<const uint32_t *>(idxBufferData);
							for (uint32_t idx = 0; idx < indexCount; idx++) {
								data.indices.push_back(castData[idx] + vertexStart);
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
							const uint16_t *castData = static_cast<const uint16_t *>(idxBufferData);
							for (uint32_t idx = 0; idx < indexCount; idx++) {
								data.indices.push_back(static_cast<uint32_t>(castData[idx]) + vertexStart);
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
							const uint8_t *castData = static_cast<const uint8_t *>(idxBufferData);
							for (uint32_t idx = 0; idx < indexCount; idx++) {
								data.indices.push_back(static_cast<uint32_t>(castData[idx]) + vertexStart);
							}
						}
						break;
						default:
							throw std::runtime_error("Attempted to load model indices with an unsupported data type. Supported: uint32, uint16, uint8");
					}

				}
				
				// Assign primitive to mesh
				std::shared_ptr<Primitive> createPrimitive = std::make_shared<Primitive>(
					indexStart, 
					indexCount, 
					vertexCount, 
					primitive.material > -1 ? data.materials[primitive.material].get() : data.materials.back().get(),
					primitive.material > -1 ? primitive.material : data.materials.size() - 1
				);
				createMesh->primitives.push_back(std::move(createPrimitive));
			}
			// Assign mesh to node
			createNode->mesh = std::move(createMesh);
		}

		// Update node tree
		if (parent) 
			parent->children.push_back(createNode.get());
		else
			data.nodes.push_back(createNode);
		
		// Flattened node tree for skinning
		data.flatNodes.push_back(createNode);
	}

	std::shared_ptr<SumiModel::Node> SumiModel::getGLTFnode(uint32_t idx, SumiModel::Data &data) {
		// TODO: This linear (O(n)) search is pretty slow for objects with lots of nodes.
		//		 Would recommend making another field for data: nodeMap which has <idx, Node*> pairs
		//		 to reduce this to O(1).
		for (auto& node : data.flatNodes) {
			if (node->idx == idx) {
				return node;
			}
		}
		return nullptr;
	}

	//==============Vertex======================================================================
	
	std::vector<VkVertexInputBindingDescription> SumiModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> SumiModel::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({0, 0 , VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, joint)});
		attributeDescriptions.push_back({1, 0 , VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight)});
		attributeDescriptions.push_back({2, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
		attributeDescriptions.push_back({3, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
		attributeDescriptions.push_back({4, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
		attributeDescriptions.push_back({5, 0 , VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)});

		return attributeDescriptions;
	}

	//==============Node========================================================================

	// local transform matrix for a *single* node
	glm::mat4 SumiModel::Node::getLocalTransform() {
		// TODO: potentially use sumi_transform3d.hpp here for consistency.
		return glm::translate(glm::mat4{1.0f}, translation) * glm::mat4(rotation) * glm::scale(glm::mat4{1.0f}, scale) * matrix;
	}

	// global transform matrix for a node (considering its parents transforms)
	glm::mat4 SumiModel::Node::getGlobalTransform() {

		glm::mat4 globalMatrix = getLocalTransform();
		Node *parentNode = parent;

		// Recurse through parents and accumulate transforms
		while (parentNode) {
			globalMatrix = parentNode->getLocalTransform() * globalMatrix;
			parentNode = parentNode->parent;
		}

		return globalMatrix;
	}

	void SumiModel::Node::update() {
		// TODO: THIS UPDATE IS WHAT IS CAUSING ANIMATION PROBLEMS UH OHHHH POOOOPY STINKY AAAH CODE
		// Update mesh nodes
		if (mesh) {
			// Update node matrix
			glm::mat4 nodeMatrix = getGlobalTransform();
			mesh->uniforms.matrix = nodeMatrix;

			// Update joint matrix
			if (skin) {
				glm::mat4 inverseTransform = glm::inverse(nodeMatrix);
				size_t nJoints = std::min(static_cast<uint32_t>(skin->joints.size()), MODEL_MAX_JOINTS);
				for (size_t i = 0; i < nJoints; i++) {
					Node *jointNode = skin->joints[i];
					glm::mat4 jointMat = jointNode->getGlobalTransform() * skin->inverseBindMatrices[i];
					jointMat = inverseTransform * jointMat;
					mesh->uniforms.jointMatrices[i] = jointMat;
				}
				mesh->uniforms.nJoints = static_cast<int>(nJoints);

				// Write updated uniforms
				mesh->uniformBuffer->writeToBuffer(&mesh->uniforms);
				mesh->uniformBuffer->flush();
			} else {
				mesh->uniformBuffer->writeToBuffer(&mesh->uniforms.matrix, sizeof(glm::mat4), 0);
				mesh->uniformBuffer->flush();
			}
		}

		// Update Children
		for (auto& child : children) {
			child->update();
		}
	}

	//==============Mesh========================================================================

	SumiModel::Mesh::Mesh(SumiDevice &device, glm::mat4 matrix) {
		// Set uniforms
		uniforms.matrix = matrix;
		
		// Create mesh uniform buffer and descriptor
		// TODO: Maybe use host coherent here for ease of update
		uniformBuffer = std::make_unique<SumiBuffer>(
			device,
			sizeof(Mesh::UniformData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);
		uniformBuffer->map();
		uniformBuffer->writeToBuffer(&uniforms); // initial buffer write
		uniformBuffer->flush();

		// Descriptor is left unanitialized until the model initializes it.
	}

}