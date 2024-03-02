#include <sumire/core/materials/sumi_material.hpp>

#include <sumire/core/sumi_swap_chain.hpp>

namespace sumire {

    // TODO: Move this constructor to header for clarity if it remains empty
    SumiMaterial::SumiMaterial(id_t matId, SumiDevice &device, MaterialTextureData &data) 
        : id{matId}, texData{data} {}
    
    SumiMaterial::~SumiMaterial() {}

    // Writes the material's texture descriptors to the given set.
    void SumiMaterial::writeDescriptorSet(
        SumiDescriptorPool &descriptorPool, 
        SumiDescriptorSetLayout &layout,
        SumiTexture *defaultTexture
    ) {
        // If a texture isn't defined, use a default texture instead 
        std::vector<VkDescriptorImageInfo> imageDescriptors = {
            texData.baseColorTexture ? 
                texData.baseColorTexture->getDescriptorInfo() : defaultTexture->getDescriptorInfo(),
            texData.metallicRoughnessTexture ?
                texData.metallicRoughnessTexture->getDescriptorInfo() : defaultTexture->getDescriptorInfo(),
            texData.normalTexture ? 
                texData.normalTexture->getDescriptorInfo() : defaultTexture->getDescriptorInfo(),
            texData.occlusionTexture ?
                texData.occlusionTexture->getDescriptorInfo() : defaultTexture->getDescriptorInfo(),
            texData.emissiveTexture ?
                texData.emissiveTexture->getDescriptorInfo()  : defaultTexture->getDescriptorInfo()
        };

        assert(SumiMaterial::MAT_TEX_COUNT == imageDescriptors.size() && "Material Descriptor layout does not match texture descriptor count (MAT_TEX_COUNT may need updating)");
        
        SumiDescriptorWriter writer{layout, descriptorPool};
        for (size_t i = 0; i < imageDescriptors.size(); i++) {
            writer.writeImage(static_cast<uint32_t>(i), &imageDescriptors[i]);
        }
        writer.build(matDescriptorSet);
    }

    // TODO: Make this class interface through a material manager first before returning.
    std::unique_ptr<SumiDescriptorSetLayout> SumiMaterial::getDescriptorSetLayout(SumiDevice &device) {

        assert(SumiMaterial::MAT_TEX_COUNT == 5 && "Material Descriptor layout does not match texture count (MAT_TEX_COUNT may need updating)");

        return SumiDescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

    SumiMaterial::MaterialShaderData SumiMaterial::getMaterialShaderData() {
        return MaterialShaderData{
            texData.baseColorFactors,
            texData.emissiveFactors,
            texData.metallicRoughnessFactors,
            texData.normalScale,
            texData.occlusionStrength,
            texData.baseColorTexCoord,
            texData.metallicRoughnessTexCoord,
            texData.normalTexCoord,
            texData.occlusionTexCoord,
            texData.emissiveTexCoord,
            texData.alphaMode == AlphaMode::MODE_MASK,
            texData.alphaCutoff
        };
    }

}