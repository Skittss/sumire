#pragma once

#include <sumire/core/models/sumi_model.hpp>

namespace sumire::loaders {

    class OBJloader {
        public:
            static std::unique_ptr<SumiModel> createModelFromFile(
                SumiDevice &device, 
                const std::string &filepath, 
                bool genTangents = true
            );
        
        private:
            static void loadModel(
                SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool genTangents);
            static void loadOBJ(
                SumiDevice &device, const std::string &filepath, SumiModel::Data &data, bool genTangents);
            
            // TODO: Move this to a material manager
            static std::unique_ptr<SumiMaterial> createDefaultMaterial(SumiDevice &device);

    };

}