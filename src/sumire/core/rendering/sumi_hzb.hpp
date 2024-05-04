#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

namespace sumire {

    class SumiHZB {
    public:
        enum HierarchyType {
            MIP_CHAIN,
            SHADOW_TILE_8X8
        };

        SumiHZB(
            SumiDevice& device, 
            SumiAttachment* zbuffer,
            VkImageUsageFlags usageFlags,
            SumiHZB::HierarchyType heirarchyType,
            uint32_t mipLevels = 1
        );
        ~SumiHZB();

        VkImage getImage() const { return image; }
        VkImageView getBaseImageView() const { return baseImageView; }
        VkImageViewCreateInfo getBaseImageViewCreateInfo() const { return baseImageViewInfo; }
        VkExtent2D getBaseExtent() const { return baseExtent; }


    private:
        void createHZBimage(VkImageUsageFlags usageFlags);
        void createMippedHZBimage(VkImageUsageFlags usageFlags);
        void createShadowTileHZBimage(VkImageUsageFlags usageFlags);

        void createBaseImageView();

        SumiDevice& sumiDevice;
        const HierarchyType heirarchyType;

        const VkFormat format = VK_FORMAT_R16_UNORM;
        const VkExtent2D zbufferResolution;
        const uint32_t mipLevels;

        VkDeviceMemory memory     = VK_NULL_HANDLE;
        VkImage image             = VK_NULL_HANDLE;
        VkImageView baseImageView = VK_NULL_HANDLE;
        VkExtent2D baseExtent;


        VkImageViewCreateInfo baseImageViewInfo;

    };

}