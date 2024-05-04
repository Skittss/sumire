#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

namespace sumire {

    class SumiHZB {
    public:
        enum HeirarchyType {
            MIP_CHAIN,
            SINGLE_IMAGE
        };

        SumiHZB(
            SumiDevice& device, 
            SumiAttachment* zbuffer,
            VkImageUsageFlags usageFlags,
            SumiHZB::HeirarchyType heirarchyType,
            uint32_t mipLevels = 0,
            uint32_t singleImageLevel = 0
        );
        ~SumiHZB();

        VkImage getImage() const { return image; }
        VkImageView getBaseImageView() const { return baseImageView; }
        VkImageViewCreateInfo getBaseImageViewCreateInfo() const { return baseImageViewInfo; }

    private:
        void createHZBimage(VkImageUsageFlags usageFlags);
        void createMippedHZBimage(VkImageUsageFlags usageFlags);
        void createSingleHZBimage(VkImageUsageFlags usageFlags);

        void createBaseImageView();

        SumiDevice& sumiDevice;
        const HeirarchyType heirarchyType;

        const VkFormat format = VK_FORMAT_R16_UNORM;
        const VkExtent2D zbufferResolution;
        const uint32_t mipLevels;
        const uint32_t singleImageLevel;

        VkDeviceMemory memory     = VK_NULL_HANDLE;
        VkImage image             = VK_NULL_HANDLE;
        VkImageView baseImageView = VK_NULL_HANDLE;

        VkImageViewCreateInfo baseImageViewInfo;

    };

}