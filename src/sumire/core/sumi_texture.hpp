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
                VkSamplerCreateInfo &samplerInfo,
                SumiBuffer &imageStagingBuffer
            );
            ~SumiTexture();

            static std::unique_ptr<SumiTexture> createFromFile(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                const std::string &filepath
            );
            static std::unique_ptr<SumiTexture> createFromRGBA(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                int width, int height, unsigned char *data
            );
            static std::unique_ptr<SumiTexture> createFromRGB(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                int width, int height, unsigned char *data
            );
            static void defaultImageCreateInfo(VkImageCreateInfo &createInfo);
            static void defaultSamplerCreateInfo(SumiDevice &device, VkSamplerCreateInfo &createInfo);

            VkDescriptorImageInfo& getDescriptorInfo() { return descriptorInfo; }

        private:

            void createTextureImage(
                VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo,
                SumiBuffer &stagingBuffer
            );
            void createTextureImageView(VkFormat format);
            void createTextureSampler(VkSamplerCreateInfo &samplerInfo);
            void writeDescriptorInfo();

            SumiDevice &sumiDevice;

            // Image, Sampler and Memory
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            VkMemoryPropertyFlags memoryPropertyFlags;

            // Descriptor
            VkDescriptorImageInfo descriptorInfo;
    };
    
}