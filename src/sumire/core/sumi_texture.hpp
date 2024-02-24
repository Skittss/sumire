# pragma once

#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_buffer.hpp>

#include <memory>

namespace sumire {

    class SumiTexture {
        public:

            struct TextureConfigInfo {
                uint32_t mipmapLevels{1};
            };

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
            static std::unique_ptr<SumiTexture> createFromRGBA(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
                int width, int height, unsigned char *data
            );
            static std::unique_ptr<SumiTexture> createFromRGB(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
                int width, int height, unsigned char *data
            );
            static void defaultImageCreateInfo(VkImageCreateInfo &createInfo);

        private:

            void createTextureImage(
                VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo,
                SumiBuffer &stagingBuffer
            );
            void createTextureImageView(VkFormat format);
            void createTextureSampler();

            SumiDevice &sumiDevice;

            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            VkMemoryPropertyFlags memoryPropertyFlags;
    };
    
}