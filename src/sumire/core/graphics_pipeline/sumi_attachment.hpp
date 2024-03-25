#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <vulkan/vulkan.h>

namespace sumire {

    class SumiAttachment {
        public:

            SumiAttachment(
                SumiDevice &device, 
                VkExtent2D extent,
                VkFormat format, 
                VkImageUsageFlags usage
            );
            // Construct attachment with an existing VkImage handle
            SumiAttachment(
                SumiDevice &device, 
                VkImage image,
                VkFormat format,
                VkImageAspectFlags aspectMask
            );

            ~SumiAttachment();

            VkFormat getFormat() const { return format; }
            VkImageView getImageView() const { return view; }

        private:
            void createImage(VkExtent2D extent, VkImageUsageFlags usage);
            void createImageView(VkImageAspectFlags aspectMask);

            SumiDevice& sumiDevice;

            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkFormat format;

    };

}