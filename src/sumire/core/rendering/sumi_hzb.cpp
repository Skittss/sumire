#include <sumire/core/rendering/sumi_hzb.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {
    
    SumiHZB::SumiHZB(
        SumiDevice& device, 
        SumiAttachment* zbuffer,
        VkImageUsageFlags usageFlags,
        SumiHZB::HeirarchyType heirarchyType,
        uint32_t mipLevels,
        uint32_t singleImageLevel
    ) : sumiDevice{ device }, 
        heirarchyType{ heirarchyType }, 
        zbufferResolution{ zbuffer->getExtent()},
        mipLevels{ mipLevels },
        singleImageLevel{ singleImageLevel }
    {
        createHZBimage(usageFlags);
        createBaseImageView();
    }

    SumiHZB::~SumiHZB() {
        vkDestroyImageView(sumiDevice.device(), baseImageView, nullptr);
        vkDestroyImage(sumiDevice.device(), image, nullptr);
    }

    void SumiHZB::createHZBimage(VkImageUsageFlags usageFlags) {
        if (heirarchyType == HeirarchyType::MIP_CHAIN) {
            createMippedHZBimage(usageFlags);
        }
        else {
            createSingleHZBimage(usageFlags);
        }
    }

    void SumiHZB::createMippedHZBimage(VkImageUsageFlags usageFlags) {

        uint32_t downscaledX = zbufferResolution.width  << 1;
        uint32_t downscaledY = zbufferResolution.height << 1;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = downscaledX;
        imageInfo.extent.height = downscaledY;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = mipLevels;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usageFlags;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags         = 0x0;

        sumiDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            memory
        );
    }

    void SumiHZB::createSingleHZBimage(VkImageUsageFlags usageFlags) {

        uint32_t downscaledX = zbufferResolution.width  << singleImageLevel;
        uint32_t downscaledY = zbufferResolution.height << singleImageLevel;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = downscaledX;
        imageInfo.extent.height = downscaledY;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usageFlags;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags         = 0x0;

        sumiDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            memory
        );
    }

    void SumiHZB::createBaseImageView() {
        baseImageViewInfo = VkImageViewCreateInfo{};
        baseImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        baseImageViewInfo.image = image;
        baseImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        baseImageViewInfo.format = format;
        baseImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        baseImageViewInfo.subresourceRange.baseMipLevel = 0;
        baseImageViewInfo.subresourceRange.levelCount = 1;
        baseImageViewInfo.subresourceRange.baseArrayLayer = 0;
        baseImageViewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_SUCCESS(
            vkCreateImageView(sumiDevice.device(), &baseImageViewInfo, nullptr, &baseImageView),
            "[Sumire::SumiGbuffer] Failed to create attachment image view"
        );
    }

}