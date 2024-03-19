#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <sumire/loaders/obj_loader.hpp>

#include <sumire/math/math_utils.hpp>
#include <sumire/util/generate_mikktspace_tangents.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <filesystem>
#include <unordered_map>

namespace std {
    // Allow hashing of Vertex
	template <>
	struct hash<sumire::Vertex> {
		size_t operator()(sumire::Vertex const &vertex) const {
			size_t seed = 0;
			sumire::hashCombine(seed,
				vertex.joint,
				vertex.weight,
				vertex.position, 
				vertex.color, 
				vertex.normal, 
				vertex.tangent,
				vertex.uv0, 
				vertex.uv1
			);
			return seed;
		}
	};
}

namespace sumire::loaders {

    std::unique_ptr<SumiModel> OBJloader::createModelFromFile(
		SumiDevice &device, 
		const std::string &filepath,
		bool genTangents
	) {
        std::filesystem::path fp = filepath;
        SumiModel::Data data{};
        loadModel(device, filepath, data, genTangents);

        auto modelPtr = std::make_unique<SumiModel>(device, data);
        modelPtr->displayName = fp.filename().u8string();

        std::cout << "Loaded Model <" << filepath << "> (verts: " << data.vertices.size() 
					<< ", triangles: " << (modelPtr->hasIndices() ? data.indices.size() / 3.0f : data.vertices.size())
                    << ", nodes: " << data.flatNodes.size() << " [top level: " << data.nodes.size() << "]"
                    << ", mat: " << data.materials.size()
                    << ", tex: " << data.textures.size()
                    << ")" << std::endl;
        return modelPtr;
    }

    void OBJloader::loadModel(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool genTangents) {
		std::filesystem::path fp = filepath;
		std::filesystem::path ext = fp.extension();

		if (ext == ".obj") 
			loadOBJ(device, filepath, data, genTangents);
		else
			throw std::runtime_error("[Sumire::OBJloader] Attempted to load unsupported OBJ type: <" + ext.u8string() + ">");

	}

    void OBJloader::loadOBJ(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool genTangents) {
		// .Obj loading
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			throw std::runtime_error("[Sumire::OBJloader] " + warn + err);
		}

		data.vertices.clear();
		data.indices.clear();

		std::unique_ptr<Node> mainNode = std::make_unique<Node>();
		mainNode->idx = 0;
		mainNode->parent = nullptr;
		mainNode->name = "mesh";
		mainNode->mesh = std::make_unique<Mesh>(device, glm::mat4{1.0});

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

				vertex.tangent = {0.0f, 0.0f, 0.0f, 0.0f};

				if (index.texcoord_index >= 0) {
					vertex.uv0 = {
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

		uint32_t vertexCount = data.vertices.size();
		uint32_t indexCount = data.indices.size();

		// Tangent generation
		if (genTangents) {
			util::MikktspaceData mikktspaceData{
				data.vertices,
				data.indices,
				std::vector<glm::vec4>(vertexCount),
				0,
				vertexCount,
				0,
				indexCount
			};

			util::generateMikktspaceTangents(&mikktspaceData);

			for (uint32_t vIdx = 0; vIdx < vertexCount; vIdx++) {
				data.vertices[vIdx].tangent = mikktspaceData.outTangents[vIdx];
			}
		}

		// Push default material
		data.materials.push_back(OBJloader::createDefaultMaterial(device));

		std::unique_ptr<Primitive> mainPrimitive = std::make_unique<Primitive>(
			0, 
			indexCount, 
			vertexCount, 
			data.materials.back().get(), 
			0 // Use default material only
		);
		mainNode->mesh->primitives.push_back(std::move(mainPrimitive));
		data.meshCount = 1;

		data.flatNodes.push_back(std::move(mainNode));
		data.nodes.push_back(data.flatNodes.back().get());
	}

    std::unique_ptr<SumiMaterial> OBJloader::createDefaultMaterial(SumiDevice &device) {
		SumiMaterial::MaterialTextureData matData{};
		return SumiMaterial::createMaterial(device, matData);
	}
}