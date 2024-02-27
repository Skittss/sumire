#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION // No exceptions (for now?)

#include <sumire/core/sumi_model.hpp>

#include <sumire/core/sumi_swap_chain.hpp>
#include <sumire/util/gltf_vulkan_flag_converters.hpp>
#include <sumire/math/math_utils.hpp>

// TODO: Could we find a way around using experimental GLM hashing? (though it seems stable)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	}

	SumiModel::~SumiModel() {
		materialDescriptorPool = nullptr;
		meshNodeDescriptorSetLayout = nullptr;
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

		std::cout << "Loaded Model <" << filepath << "> (Vertex count: " << data.vertices.size() << ")" << std::endl;
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
		meshNodeDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

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

	void SumiModel::bind(VkCommandBuffer commandBuffer) {
		// Vertex and Index Buffers
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (useIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void SumiModel::drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer) {
		// Draw this node's primitives
		if (node->mesh) {
			for (auto& primitive : node->mesh->primitives) {
				// TODO: Case if primitive does not use indices.
				vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
			}
		}

		// Draw children
		for (auto& child : node->children) {
			drawNode(child, commandBuffer);
		}
	}

	void SumiModel::draw(VkCommandBuffer commandBuffer) {
		for (auto& node : modelData.nodes) {
			drawNode(node, commandBuffer);
		}
	}

	std::vector<VkVertexInputBindingDescription> SumiModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> SumiModel::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({0, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
		attributeDescriptions.push_back({1, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
		attributeDescriptions.push_back({2, 0 , VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
		attributeDescriptions.push_back({3, 0 , VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)});

		return attributeDescriptions;
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

		// TODO: OBJs currently have no materials causing validation errors.
		//		 Make a default material and push to material arr.
		std::shared_ptr<Primitive> mainPrimitive = std::make_shared<Primitive>(0, data.indices.size(), data.vertices.size(), nullptr);
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
		if (gltfModel.defaultScene < 0)
			std::cerr << "WARN: Model <" << filepath << "> has no default scene - here be dragons!" << std::endl;

		const int default_scene_idx = std::max(gltfModel.defaultScene, 0);
		const tinygltf::Scene &scene = gltfModel.scenes[default_scene_idx];

		// Textures
		loadGLTFsamplers(device, gltfModel, data);
		loadGLTFtextures(device, gltfModel, data);
		loadGLTFmaterials(device, gltfModel, data);

		// Mesh information
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		for (uint32_t i = 0; i < scene.nodes.size(); i++) {
			getGLTFnodeProperties(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount, data);
		}

		// Mesh buffers
		for (uint32_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadGLTFnode(device, nullptr, node, scene.nodes[i], gltfModel, data);
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
			}
			if (material.pbrMetallicRoughness.metallicRoughnessTexture.index > -1) {
				mat.metallicRoughnessTexture = data.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
			}
			if (material.normalTexture.index > -1) {
				mat.normalTexture = data.textures[material.normalTexture.index];
			}
			if (material.emissiveTexture.index > -1) {
				mat.emissiveTexture = data.textures[material.emissiveTexture.index];
			}
			if (material.occlusionTexture.index > -1) {
				mat.occlusionTexture = data.textures[material.occlusionTexture.index];
			}

			// Factors
			mat.baseColorFactors = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
			mat.metallicRoughnessFactors = {
				material.pbrMetallicRoughness.metallicFactor,
				material.pbrMetallicRoughness.roughnessFactor
			};
			mat.emissiveFactors = glm::make_vec3(material.emissiveFactor.data());

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

		// TODO: Default material to the back of the vector
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
		std::shared_ptr<Node> parent, const tinygltf::Node &node, uint32_t nodeIdx, 
		const tinygltf::Model &model, 
		SumiModel::Data &data
	) {

		std::shared_ptr<Node> createNode = std::make_shared<Node>();
		createNode->idx = nodeIdx;
		createNode->name = node.name;
		createNode->matrix = glm::mat4(1.0f);

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

		// Load node children if exists
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				loadGLTFnode(device, createNode, model.nodes[node.children[i]], node.children[i], model, data);
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

				// Vertices
				{
					// Buffer pointers & data strides
					const float *bufferPos = nullptr;
					const float *bufferNorm = nullptr;
					const float *bufferTexCoord0 = nullptr;
					const float *bufferTexCoord1 = nullptr;
					const float *bufferColor0 = nullptr;
					int stridePos;
					int strideNorm;
					int strideTexCoord0;
					int strideTexCoord1;
					int strideColor0;

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

					// Populate Vertex structs
					for (uint32_t vIdx = 0; vIdx < vertexCount; vIdx++) {
						Vertex v{};
						v.position = glm::make_vec3(&bufferPos[vIdx * stridePos]);
						v.normal = glm::normalize(bufferNorm ? glm::make_vec3(&bufferNorm[vIdx * strideNorm]) : glm::vec3(0.0f));
						v.uv = bufferTexCoord0 ? glm::make_vec2(&bufferTexCoord0[vIdx * strideTexCoord0]) : glm::vec3(0.0f);
						v.color = bufferColor0 ? glm::make_vec3(&bufferColor0[vIdx * strideColor0]) : glm::vec3(1.0f);
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
					primitive.material > -1 ? data.materials[primitive.material].get() : data.materials.back().get()
				);
				createMesh->primitives.push_back(std::move(createPrimitive));
			}
			// Assign mesh to node
			createNode->mesh = std::move(createMesh);
		}

		// Update node tree
		if (parent) 
			parent->children.push_back(createNode);
		else
			data.nodes.push_back(createNode);
		
		// Flattened node tree for skinning
		data.flatNodes.push_back(createNode);
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
		std::shared_ptr<Node> parentNode = parent;

		// Recurse through parents and accumulate transforms
		while (parentNode) {
			globalMatrix = parentNode->getLocalTransform() * globalMatrix;
			parentNode = parentNode->parent;
		}

		return globalMatrix;
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