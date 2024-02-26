#pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_pipeline.hpp>
#include <sumire/core/sumi_texture.hpp>
#include <sumire/core/sumi_descriptors.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sumire {

    class SumiMaterial {
        public:
            using id_t = unsigned int;

            // TODO: Should the material also jointly own textures, or should they all be 
            //       re-used through some texture manager to allow for better caching?
            // TODO: As a result of the above, textures in the struct below should be non-owning pointers.

            static constexpr int MAT_TEX_COUNT = 5;

            enum AlphaMode { MODE_OPAQUE, MODE_BLEND, MODE_MASK };

            // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials
            struct MaterialTextureData {
                // PBR Metallic Roughness
                std::shared_ptr<SumiTexture> baseColorTexture;
                glm::vec4 baseColorFactors;
                std::shared_ptr<SumiTexture> metallicRoughnessTexture;
                glm::vec2 metallicRoughnessFactors;
                // Lighting properties
                std::shared_ptr<SumiTexture> normalTexture;
                std::shared_ptr<SumiTexture> occlusionTexture;
                std::shared_ptr<SumiTexture> emissiveTexture;
                glm::vec3 emissiveFactors;
                // Other properties
                bool doubleSided;
                AlphaMode alphaMode;
                float alphaCutoff;
                std::string name{"Unnamed Material"};
            };

            // TODO: This constructor should be private but messes with make_unique().
            SumiMaterial(id_t matId, SumiDevice &device, MaterialTextureData &data);
            ~SumiMaterial();

            SumiMaterial(const SumiMaterial &) = delete;
            SumiMaterial &operator=(const SumiMaterial &) = delete;
            SumiMaterial(SumiMaterial&&) = default;
            SumiMaterial &operator=(SumiMaterial&&) = default;

            const id_t getId() { return id; }

            static std::unique_ptr<SumiMaterial> createMaterial(SumiDevice &device, MaterialTextureData &data) {
                static id_t currentId = 0;
                return std::make_unique<SumiMaterial>(currentId++, device, data);
            }

            static std::unique_ptr<SumiDescriptorSetLayout> getDescriptorSetLayout(SumiDevice &device);

            void writeDescriptorSet(SumiDescriptorPool &descriptorPool, SumiDescriptorSetLayout &layout);

        private:
            id_t id;
            
            MaterialTextureData texData;
            VkDescriptorSet matDescriptorSet = VK_NULL_HANDLE;
    };

}