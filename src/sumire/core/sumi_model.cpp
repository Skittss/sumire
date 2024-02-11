#include <sumire/core/sumi_model.hpp>

#include <sumire/math/math_utils.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// TODO: Could we find a way around using experimental GLM hashing? (though it seems stable)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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

	SumiModel::SumiModel(SumiDevice& device, const SumiModel::Data& data) : sumiDevice{ device } {
		createVertexBuffers(data.vertices);
		createIndexBuffer(data.indices);
	}

	SumiModel::~SumiModel() {}

	std::unique_ptr<SumiModel> SumiModel::createFromFile(SumiDevice &device, const std::string &filepath) {
		Data data{};
		loadModel(filepath, data);

		// TODO: Remove this output and iostream include.
		std::cout << "Loaded Model <" << filepath << "> (Vertex count: " << data.vertices.size() << ")" << std::endl;
		return std::make_unique<SumiModel>(device, data);
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

	void SumiModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (useIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void SumiModel::draw(VkCommandBuffer commandBuffer) {
		if (useIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		} else {
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
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

	void SumiModel::loadModel(const std::string &filepath, SumiModel::Data &data) {
		std::filesystem::path fp = filepath;
		std::filesystem::path ext = fp.extension();

		if (ext == ".obj") 
			loadOBJ(filepath, data);
		else if (ext == ".gltf") 
			loadGLTF(filepath, data);
		else 
			throw std::runtime_error("Attempted to load unsupported model type: <" + ext.u8string() + ">");

	}

	void SumiModel::loadOBJ(const std::string &filepath, SumiModel::Data &data) {
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
	}

	void SumiModel::loadGLTF(const std::string &filepath, SumiModel::Data &data) {
		
	}
}