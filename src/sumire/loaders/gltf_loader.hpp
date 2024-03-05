#pragma once

#include <sumire/core/models/sumi_model.hpp>

#include <tiny_gltf.h>

namespace sumire::loaders {

    class GLTFloader {
        public:
            static std::unique_ptr<SumiModel> createModelFromFile(SumiDevice &device, const std::string &filepath);

        private:

		    static void loadModel(SumiDevice &device, const std::string &filepath, SumiModel::Data &data);

            // TODO: For full GLTF support (including extensions), the loader should really load an entire scene
            // 		 Not just directly to a model.
            static void loadGLTF(SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool isBinaryFile);
            static void loadGLTFsamplers(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
            static void loadGLTFtextures(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
            static void loadGLTFmaterials(SumiDevice &device, tinygltf::Model &model, SumiModel::Data &data);
            static void loadGLTFskins(tinygltf::Model &model, SumiModel::Data &data);
            static void loadGLTFanimations(tinygltf::Model &model, SumiModel::Data &data);
            static void getGLTFnodeProperties(
                const tinygltf::Node &node, const tinygltf::Model &model, 
                uint32_t &vertexCount, uint32_t &indexCount,
                SumiModel::Data &data
            );
            static void loadGLTFnode(
                SumiDevice &device, 
                SumiModel::Node *parent, const tinygltf::Node &node, uint32_t nodeIdx, 
                const tinygltf::Model &model, 
                SumiModel::Data &data
            );
            static std::shared_ptr<SumiModel::Node> getGLTFnode(uint32_t idx, SumiModel::Data &data);
            static uint32_t getLowestUnreservedGLTFNodeIdx(SumiModel::Data &data);

            // TODO: Move this to a material manager
            static std::unique_ptr<SumiMaterial> createDefaultMaterial(SumiDevice &device);
    };



}