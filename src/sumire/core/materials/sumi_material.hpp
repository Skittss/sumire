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
                glm::vec4 baseColorFactors{1.0f};
                int baseColorTexCoord{-1};
                std::shared_ptr<SumiTexture> metallicRoughnessTexture;
                glm::vec2 metallicRoughnessFactors{1.0f};
                int metallicRoughnessTexCoord{-1};
                // Lighting properties
                std::shared_ptr<SumiTexture> normalTexture;
                float normalScale{1.0f};
                int normalTexCoord{-1};
                std::shared_ptr<SumiTexture> occlusionTexture;
                float occlusionStrength{1.0f};
                int occlusionTexCoord{-1};
                std::shared_ptr<SumiTexture> emissiveTexture;
                glm::vec3 emissiveFactors{1.0f};
                int emissiveTexCoord{-1};
                // Other properties
                bool doubleSided{true};
                AlphaMode alphaMode{AlphaMode::MODE_OPAQUE};
                float alphaCutoff{0.0f};
                std::string name{"Unnamed Material"};
            };

            struct MaterialShaderData {
                glm::vec4 baseColorFactors;
                alignas(16) glm::vec3 emissiveFactors;
                glm::vec2 metallicRoughnessFactors;
                float normalScale;
                float occlusionStrength;
                int baseColorTexCoord; // Textures are empty if texcoord < 0
                int metallicRoughnessTexCoord;
                int normalTexCoord;
                int occlusionTexCoord;
                int emissiveTexCoord;
                bool useAlphaMask;
                float alphaMaskCutoff;
            };
            
            // TODO: This constructor should be private but messes with make_unique().
            SumiMaterial(id_t matId, SumiDevice &device, MaterialTextureData &data);
            ~SumiMaterial();

            SumiMaterial(const SumiMaterial &) = delete;
            SumiMaterial &operator=(const SumiMaterial &) = delete;

            const id_t getId() { return id; }

            static std::unique_ptr<SumiMaterial> createMaterial(SumiDevice &device, MaterialTextureData &data) {
                static id_t currentId = 0;
                return std::make_unique<SumiMaterial>(currentId++, device, data);
            }

            static std::unique_ptr<SumiDescriptorSetLayout> getDescriptorSetLayout(SumiDevice &device);

            void writeDescriptorSet(
                SumiDescriptorPool &descriptorPool, 
                SumiDescriptorSetLayout &layout,
                SumiTexture *defaultTexture
            );

            MaterialShaderData getMaterialShaderData();
            VkDescriptorSet getDescriptorSet() { return matDescriptorSet; }

        private:
            id_t id;
            
            MaterialTextureData texData;
            VkDescriptorSet matDescriptorSet = VK_NULL_HANDLE;
    };

}