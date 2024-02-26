#include <sumire/core/materials/sumi_material.hpp>

#include <sumire/core/sumi_swap_chain.hpp>

namespace sumire {

    // TODO: Move this constructor to header for clarity if it remains empty
    SumiMaterial::SumiMaterial(id_t matId, SumiDevice &device, MaterialTextureData &data) 
        : id{matId}, texData{data} {}
    
    SumiMaterial::~SumiMaterial() {}

    // Writes the material's texture descriptors to the given set.
    void SumiMaterial::writeDescriptorSet(
        SumiDescriptorPool &descriptorPool, SumiDescriptorSetLayout &layout
    ) {

        // TODO: If a texture isn't defined, use a default texture instead 
        //      (e.g. purple missing squares / full black or white). Needs loading in and managing.
        std::vector<VkDescriptorImageInfo> imageDescriptors = {
            texData.baseColorTexture->getDescriptorInfo(),
            texData.metallicRoughnessTexture->getDescriptorInfo(),
            texData.normalTexture->getDescriptorInfo(),
            texData.occlusionTexture->getDescriptorInfo(),
            texData.emissiveTexture->getDescriptorInfo()
        };

        assert(SumiMaterial::MAT_TEX_COUNT == imageDescriptors.size() && "Material Descriptor layout does not match texture descriptor count (MAT_TEX_COUNT may need updating)");
        
        for (size_t i = 0; i < imageDescriptors.size(); i++) {
            SumiDescriptorWriter(layout, descriptorPool)
                .writeImage(static_cast<uint32_t>(i), &imageDescriptors[i])
                .build(matDescriptorSet);
        }
    }

    // TODO: Make this class interface through a material manager first before returning.
    std::unique_ptr<SumiDescriptorSetLayout> SumiMaterial::getDescriptorSetLayout(SumiDevice &device) {

        assert(SumiMaterial::MAT_TEX_COUNT == 5 && "Material Descriptor layout does not match texture count (MAT_TEX_COUNT may need updating)");

        return SumiDescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

}