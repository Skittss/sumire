# pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>

#include <memory>

namespace sumire {

    class SumiTexture {
        public:

            SumiTexture(
                SumiDevice &device, 
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkImageCreateInfo &imageInfo, 
                SumiBuffer &imageStagingBuffer
            );
            ~SumiTexture();

            static std::unique_ptr<SumiTexture> createFromFile(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo, 
                const std::string &filepath
            );
            static std::unique_ptr<SumiTexture> createFromData(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
                float *data
            );
            static void defaultImageCreateInfo(VkImageCreateInfo &createInfo);

        private:

            void createTextureImage(
                VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo,
                SumiBuffer &stagingBuffer
            );

            SumiDevice &sumiDevice;

            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;

            VkMemoryPropertyFlags memoryPropertyFlags;
    };
    
}