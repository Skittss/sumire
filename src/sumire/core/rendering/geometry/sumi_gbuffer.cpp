#include <sumire/core/rendering/geometry/sumi_gbuffer.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <stdexcept>
#include <array>

namespace sumire {

    SumiGbuffer::SumiGbuffer(
        SumiDevice& device, 
        VkExtent2D extent, 
        VkImageUsageFlags extraFlags
    ) : sumiDevice{ device }, extent{ extent } {
        createAttachments(extraFlags);
    }
    
    SumiGbuffer::~SumiGbuffer() {}

    void SumiGbuffer::createAttachments(VkImageUsageFlags extraFlags) {
        position = std::make_unique<SumiAttachment>(
            sumiDevice,
            extent,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            extraFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );

        normal = std::make_unique<SumiAttachment>(
            sumiDevice,
            extent,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            extraFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );

        albedo = std::make_unique<SumiAttachment>(
            sumiDevice,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            extraFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );

        aoMetalRoughEmissive = std::make_unique<SumiAttachment>(
            sumiDevice,
            extent,
            VK_FORMAT_R8G8B8A8_UNORM,
            extraFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );
    }
    
}