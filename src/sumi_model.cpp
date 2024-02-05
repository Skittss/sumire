#include "sumi_model.hpp"

#include "math_utils.hpp"

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

	SumiModel::~SumiModel() {
		// Clean up vertex buffer
		vkDestroyBuffer(sumiDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(sumiDevice.device(), vertexBufferMemory, nullptr);

		// Clean up index buffer
		if (useIndexBuffer) {
			vkDestroyBuffer(sumiDevice.device(), indexBuffer, nullptr);
			vkFreeMemory(sumiDevice.device(), indexBufferMemory, nullptr);
		}
	}

	std::unique_ptr<SumiModel> SumiModel::createFromFile(SumiDevice &device, const std::string &filepath) {
		Data data{};
		data.loadModel(filepath);

		// TODO: Remove this output and iostream include.
		std::cout << "Loaded Model <" << filepath << "> (Vertex count: " << data.vertices.size() << ")" << std::endl;
		return std::make_unique<SumiModel>(device, data);
	}

	void SumiModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount; // vb size

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// Create Staging Buffer
		sumiDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer,
			stagingBufferMemory);

		// Populate data using CPU, and flush to GPU (automatically)
		void* data;
		vkMapMemory(sumiDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize)); // Write to host data region
		vkUnmapMemory(sumiDevice.device(), stagingBufferMemory); // Unmap CPU region to clean-up as vertex data is const

		// Vertex Buffer
		sumiDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			vertexBuffer,
			vertexBufferMemory);

		sumiDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		// Clean up staging buffer
		vkDestroyBuffer(sumiDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(sumiDevice.device(), stagingBufferMemory, nullptr);
	}

	void SumiModel::createIndexBuffer(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		useIndexBuffer = indexCount > 0;

		if (!useIndexBuffer) return; 
		
		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount; // ib size
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		// Create Staging Buffer
		sumiDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer,
			stagingBufferMemory);

		// Populate data using CPU, and flush to GPU (automatically)
		void* data;
		vkMapMemory(sumiDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize)); // Write to host data region
		vkUnmapMemory(sumiDevice.device(), stagingBufferMemory); // Unmap CPU region to clean-up as vertex data is const

		// Index Buffer
		sumiDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			indexBuffer,
			indexBufferMemory);

		sumiDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		// Clean up staging buffer
		vkDestroyBuffer(sumiDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(sumiDevice.device(), stagingBufferMemory, nullptr);
	}

	void SumiModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (useIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		return attributeDescriptions;

	}

	void SumiModel::Data::loadModel(const std::string &filepath) {

		std::filesystem::path fp = filepath;
		std::filesystem::path ext = fp.extension();

		if (ext != ".obj") {
			throw std::runtime_error("Attempted to load unsupported model type: <" + ext.u8string() + ">");
		}

		// .Obj loading
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertices.clear();
		indices.clear();

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

					// Colour support (optional)
					auto colorIdx = 3 * index.vertex_index + 2;
					if (colorIdx < attrib.colors.size()) {
						vertex.color = {
							attrib.colors[colorIdx - 2], 
							attrib.colors[colorIdx - 1], 
							attrib.colors[colorIdx - 0]
						};
					} else {
						vertex.color = {1.0f, 1.0f, 1.0f}; // default colour
					}
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
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}

	}

}