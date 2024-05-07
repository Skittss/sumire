#include <sumire/core/rendering/sumi_hzb.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {
    
    SumiHZB::SumiHZB(
        SumiDevice& device, 
        SumiAttachment* zbuffer,
        VkImageUsageFlags usageFlags,
        SumiHZB::HierarchyType heirarchyType,
        SumiHZB::DepthPoolingType depthPoolingType,
        uint32_t mipLevels
    ) : sumiDevice{ device }, 
        heirarchyType{ heirarchyType }, 
        depthPoolingType{ depthPoolingType },
        format{ depthPoolingType == DEPTH_MIN_MAX ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R16_UNORM },
        zbufferResolution{ zbuffer->getExtent()},
        mipLevels{ mipLevels }
    {
        createHZBimage(usageFlags);
        createBaseImageView();
    }

    SumiHZB::~SumiHZB() {
        vkDestroyImageView(sumiDevice.device(), baseImageView, nullptr);
        vkDestroyImage(sumiDevice.device(), image, nullptr);
        vkFreeMemory(sumiDevice.device(), memory, nullptr);
    }

    void SumiHZB::createHZBimage(VkImageUsageFlags usageFlags) {
        switch (heirarchyType) {
        case HierarchyType::MIP_CHAIN: {
            createMippedHZBimage(usageFlags);
        } break;
        case HierarchyType::SHADOW_TILE_8X8: {
            createShadowTileHZBimage(usageFlags);
        } break;
        default: {
            throw std::runtime_error("[Sumire::SumiHZB] Invalid hierarchy type provided for creation of HZB image.");
        } break;
        }
    }

    void SumiHZB::createMippedHZBimage(VkImageUsageFlags usageFlags) {
        baseExtent = VkExtent2D{ 
            zbufferResolution.width  >> 1,
            zbufferResolution.height >> 1
        };

        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = baseExtent.width;
        imageInfo.extent.height = baseExtent.height;
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

    void SumiHZB::createShadowTileHZBimage(VkImageUsageFlags usageFlags) {
        baseExtent = VkExtent2D{ 
            zbufferResolution.width  >> 3,
            zbufferResolution.height >> 3
        };

        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = baseExtent.width;
        imageInfo.extent.height = baseExtent.height;
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
        baseImageViewInfo.subresourceRange.levelCount = heirarchyType == MIP_CHAIN ? mipLevels : 1;
        baseImageViewInfo.subresourceRange.baseArrayLayer = 0;
        baseImageViewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_SUCCESS(
            vkCreateImageView(sumiDevice.device(), &baseImageViewInfo, nullptr, &baseImageView),
            "[Sumire::SumiGbuffer] Failed to create attachment image view"
        );
    }

}