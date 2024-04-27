#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION // No exceptions (for now?)

#include <sumire/loaders/gltf_loader.hpp>

#include <sumire/util/gltf_vulkan_flag_converters.hpp>
#include <sumire/util/gltf_interpolators.hpp>
#include <sumire/util/generate_mikktspace_tangents.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <unordered_set>
#include <filesystem>

namespace sumire::loaders {

    std::unique_ptr<SumiModel> GLTFloader::createModelFromFile(
        SumiDevice &device, 
        const std::string &filepath, 
        bool genTangents
    ) {
        std::filesystem::path fp = filepath;
        SumiModel::Data data{};
        loadModel(device, filepath, data, genTangents);

        auto modelPtr = std::make_unique<SumiModel>(device, data);
        modelPtr->displayName = fp.filename().u8string();

        std::cout << "[Sumire:GLTFloader] Loaded Model <" << filepath << "> (verts: " << data.vertices.size() 
                    << ", triangles: " << (modelPtr->hasIndices() ? data.indices.size() / 3.0f : data.vertices.size())
                    << ", nodes: " << data.flatNodes.size()
                    << ", mat: " << data.materials.size()
                    << ", tex: " << data.textures.size()
                    << ")" << std::endl;
        return modelPtr;
    }

    void GLTFloader::loadModel(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool genTangents) {
        std::filesystem::path fp = filepath;
        std::filesystem::path ext = fp.extension();

        if (ext == ".gltf") 
            loadGLTF(device, filepath, data, false, genTangents);
        else if (ext == ".glb")
            loadGLTF(device, filepath, data, true, genTangents);
        else
            throw std::runtime_error("[Sumire::GLTFloader] Attempted to load unsupported GLTF type: <" + ext.u8string() + ">");

    }

    void GLTFloader::loadGLTF(
        SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool isBinaryFile, bool genTangents
    ) {
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
            throw std::runtime_error("[Sumire::GLTFloader] " + warn + err);
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
            loadGLTFnode(device, nullptr, node, scene.nodes[i], gltfModel, data, genTangents);
        }
        
        // Animations
        if (gltfModel.animations.size() > 0) {
            loadGLTFanimations(gltfModel, data);
        }

        // Skinning Information
        // Note: Uses loaded nodes as joints, so ensure to always load nodes first.
        loadGLTFskins(gltfModel, data);

        // Assign skins to nodes
        for (auto& node : data.flatNodes) {
            if(node->skinIdx > -1) {
                node->skin = data.skins[node->skinIdx].get();
                // Init joint storage buffer
                node->mesh->initJointBuffer(device, node->skin->joints.size());
            }
        }

        // Initial pose
        for (auto& node : data.nodes) {
            node->applyTransformHierarchy();
            node->updateRecursive();
        }
    }

    void GLTFloader::loadGLTFsamplers(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {
        
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

    void GLTFloader::loadGLTFtextures(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {

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

    void GLTFloader::loadGLTFmaterials(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data) {
        for (tinygltf::Material &material : model.materials) {
            
            SumiMaterial::MaterialTextureData mat{};

            // Textures
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

            // Extensions
            if (material.extensions.find("KHR_materials_unlit") != material.extensions.end()) {
                mat.unlit = true;
            }

            data.materials.push_back(SumiMaterial::createMaterial(device, mat));
        }

        // Push Default material to back of material array
        data.materials.push_back(GLTFloader::createDefaultMaterial(device));
    }

    void GLTFloader::loadGLTFskins(tinygltf::Model &model, SumiModel::Data &data) {
        assert(data.nodes.size() > 0 && "Attempted to load skins before loading model nodes.");
        assert(data.flatNodes.size() > 0 && "Flattened model nodes array was uninitialized when loading skins.");
        
        static int cnt = 0;
        for (tinygltf::Skin &skin : model.skins) {
            std::unique_ptr<Skin> createSkin = std::make_unique<Skin>();
            createSkin->name = skin.name;

            // Skeleton Root Node
            if (skin.skeleton > -1) {
                createSkin->skeletonRoot = getGLTFnode(skin.skeleton, data);
            }

            // Joint Nodes
            for (int jointIdx : skin.joints) {
                Node *jointNode = getGLTFnode(jointIdx, data);
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

    void GLTFloader::loadGLTFanimations(tinygltf::Model &model, SumiModel::Data &data) {
        assert(data.flatNodes.size() > 0 && "Flattened model nodes array was uninitialized when loading animations.");

        for (tinygltf::Animation &animation : model.animations) {
            std::unique_ptr<Animation> createAnimation = std::make_unique<Animation>();
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
                createChannel.node = getGLTFnode(channel.target_node, data);
                assert(createChannel.node != nullptr && "Animation reffered to a non-existant model node");

                createAnimation->channels.push_back(createChannel);
            }

            data.animations.push_back(std::move(createAnimation));
        }
    }

    void GLTFloader::getGLTFnodeProperties(
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

    void GLTFloader::loadGLTFnode(
        SumiDevice &device,
        Node *parent, const tinygltf::Node &node, uint32_t nodeIdx, 
        const tinygltf::Model &model, 
        SumiModel::Data &data,
        bool genTangents
    ) {
        std::unique_ptr<Node> createNode = std::make_unique<Node>();
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

        // Load node children if exists
        if (node.children.size() > 0) {
            for (size_t i = 0; i < node.children.size(); i++) {
                loadGLTFnode(
                    device, createNode.get(), model.nodes[node.children[i]], node.children[i], model, data, genTangents);
            }
        }

        // Load mesh if node has it
        if (node.mesh > -1) {
            const tinygltf::Mesh mesh = model.meshes[node.mesh];
            std::unique_ptr<Mesh> createMesh = std::make_unique<Mesh>(device, createNode->matrix);
            
            for (size_t i = 0; i < mesh.primitives.size(); i++) {

                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
                uint32_t vertexStart = static_cast<uint32_t>(data.vertices.size());
                uint32_t indexStart = static_cast<uint32_t>(data.indices.size());

                const tinygltf::Primitive &primitive = mesh.primitives[i];
                bool hasIndices = primitive.indices > -1;
                bool hasSkin;

                // Vertices
                // Buffer pointers & data strides
                const float *bufferPos = nullptr;
                const float *bufferNorm = nullptr;
                const float *bufferTangent = nullptr;
                const float *bufferTexCoord0 = nullptr;
                const float *bufferTexCoord1 = nullptr;
                const float *bufferColor0 = nullptr;
                const void  *rawBufferJoints0 = nullptr;
                const float *bufferWeights0 = nullptr;
                int stridePos;
                int strideNorm;
                int strideTangent;
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

                auto tangentEntry = primitive.attributes.find("TANGENT");
                if (tangentEntry != primitive.attributes.end()) {
                    const tinygltf::Accessor& tangentAccessor = model.accessors[tangentEntry->second];
                    const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
                    const tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
                    bufferTangent = reinterpret_cast<const float *>(&(tangentBuffer.data[tangentAccessor.byteOffset + tangentBufferView.byteOffset]));
                    strideTangent = tangentAccessor.ByteStride(tangentBufferView) ? (tangentAccessor.ByteStride(tangentBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
                }

                // UVs
                auto texCoordEntry0 = primitive.attributes.find("TEXCOORD_0");
                if (texCoordEntry0 != primitive.attributes.end()) {
                    const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordEntry0->second];
                    const tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
                    const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordBufferView.buffer];
                    bufferTexCoord0 = reinterpret_cast<const float *>(&(texCoordBuffer.data[texCoordAccessor.byteOffset + texCoordBufferView.byteOffset]));
                    strideTexCoord0 = texCoordAccessor.ByteStride(texCoordBufferView) ? (texCoordAccessor.ByteStride(texCoordBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2); 
                }
                auto texCoordEntry1 = primitive.attributes.find("TEXCOORD_1");
                if (texCoordEntry1 != primitive.attributes.end()) {
                    const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordEntry1->second];
                    const tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
                    const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordBufferView.buffer];
                    bufferTexCoord1 = reinterpret_cast<const float *>(&(texCoordBuffer.data[texCoordAccessor.byteOffset + texCoordBufferView.byteOffset]));
                    strideTexCoord1 = texCoordAccessor.ByteStride(texCoordBufferView) ? (texCoordAccessor.ByteStride(texCoordBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2); 
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
                            throw std::runtime_error("[Sumire::GLTFloader] Attempted to load skin with an unsupported data type. Supported: uint16, uint8");
                    }
                }

                // Populate Vertex structs
                for (uint32_t vIdx = 0; vIdx < vertexCount; vIdx++) {
                    Vertex v{};
                    v.position = glm::make_vec3(&bufferPos[vIdx * stridePos]);
                    v.normal = glm::normalize(bufferNorm ? glm::make_vec3(&bufferNorm[vIdx * strideNorm]) : glm::vec3{0.0f});
                    v.tangent = glm::normalize(bufferTangent ? glm::make_vec4(&bufferTangent[vIdx * strideTangent]) : glm::vec4{0.0f});
                    v.uv0 = bufferTexCoord0 ? glm::make_vec2(&bufferTexCoord0[vIdx * strideTexCoord0]) : glm::vec3{0.0f};
                    v.uv1 = bufferTexCoord1 ? glm::make_vec2(&bufferTexCoord1[vIdx * strideTexCoord0]) : glm::vec3{0.0f};
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
                            throw std::runtime_error("[Sumire::GLTFloader] Attempted to load model indices with an unsupported data type. Supported: uint32, uint16, uint8");
                    }

                }

                // Generate and assign tangents using Mikktspace baking if tangents are not provided by the model
                if (genTangents && !bufferTangent) {
                    util::MikktspaceData mikktspaceData {
                        data.vertices,
                        data.indices,
                        std::vector<glm::vec4>(vertexCount),
                        vertexStart,
                        vertexCount,
                        indexStart,
                        indexCount
                    };

                    util::generateMikktspaceTangents(&mikktspaceData);

                    for (uint32_t vIdx = 0; vIdx < vertexCount; vIdx++) {
                        data.vertices[vertexStart + vIdx].tangent = mikktspaceData.outTangents[vIdx];
                    }
                }
                
                // Assign primitive to mesh
                std::unique_ptr<Primitive> createPrimitive = std::make_unique<Primitive>(
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

        // Flattened node tree for ownership of nodes and skinning
        data.flatNodes.push_back(std::move(createNode));

        // Update node tree (careful not to reference createNode as it was moved to the flatNodes vector)
        if (parent) 
            parent->children.push_back(data.flatNodes.back().get());
        else
            data.nodes.push_back(data.flatNodes.back().get());
        
    }

    Node* GLTFloader::getGLTFnode(uint32_t idx, SumiModel::Data &data) {
        // TODO: This linear (O(n)) search is pretty slow for objects with lots of nodes.
        //		 Would recommend making another field for data: nodeMap which has <idx, Node*> pairs
        //		 to reduce this to O(1).
        for (auto& node : data.flatNodes) {
            if (node->idx == idx) {
                return node.get();
            }
        }
        return nullptr;
    }

    uint32_t GLTFloader::getLowestUnreservedGLTFNodeIdx(SumiModel::Data &data) {
        // TODO: It would be beneficial to have this node idx map stored to make getGLTFnode faster as well.
        // Smallest number n not in node indices - O(2N)
        std::unordered_set<uint32_t> indices{};
        for (auto &node : data.flatNodes) {
            indices.insert(node->idx);
        }

        // Find lowest n
        // There must be some n in the range [0, size + 1] as indices are a 1-1 mapping with nodes
        for (uint32_t i = 0; i < data.flatNodes.size() + 1; i++) {
            if (indices.count(i) == 0) return i;
        }

        std::runtime_error("Could not find unreserved glTF node idx in range 0 -> " + std::to_string(data.flatNodes.size() + 1));
        return 0;
    }

    std::unique_ptr<SumiMaterial> GLTFloader::createDefaultMaterial(SumiDevice &device) {
        SumiMaterial::MaterialTextureData matData{};
        return SumiMaterial::createMaterial(device, matData);
    }

}