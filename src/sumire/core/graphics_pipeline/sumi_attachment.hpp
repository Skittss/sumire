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
                VkImageUsageFlagBits usage
            );
            ~SumiAttachment();

        private:
            void createAttachment(VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage);

            SumiDevice& sumiDevice;

            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkFormat format;

    };

}