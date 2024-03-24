#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {

    SumiAttachment::SumiAttachment(
        SumiDevice &device, 
        VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlagBits usage
    ) : sumiDevice{ device } {
        createAttachment(extent, format, usage);
    }
    
    SumiAttachment::~SumiAttachment() {
        vkDestroyImageView(sumiDevice.device(), view, nullptr);
        vkDestroyImage(sumiDevice.device(), image, nullptr);
        vkFreeMemory(sumiDevice.device(), memory, nullptr);
    }

    void SumiAttachment::createAttachment(VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage) {
        this->format = format; // For setting up render pass & potentially querying from render pipeline

        // Allocate Memory & Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0x0;

        sumiDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            image, 
            memory
        );

        VkImageAspectFlags aspectMask = 0x0;
        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) 
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        assert(aspectMask > 0 && "No suitable aspect mask provided");

        // Create Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_SUCCESS(
            vkCreateImageView(sumiDevice.device(), &viewInfo, nullptr, &view),
            "[Sumire::SumiGbuffer] Failed to create attachment image view"
        );
    }

}