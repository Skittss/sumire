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
                bool generateMips,
                VkSamplerCreateInfo &samplerInfo,
                SumiBuffer &imageStagingBuffer
            );
            ~SumiTexture();

            static std::unique_ptr<SumiTexture> createFromFile(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                const std::string &filepath,
                bool generateMips = true
            );
            static std::unique_ptr<SumiTexture> createFromRGBA(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                uint32_t width, uint32_t height, unsigned char *data,
                bool generateMips = true
            );
            static std::unique_ptr<SumiTexture> createFromRGB(
                SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
                uint32_t width, uint32_t height, unsigned char *data,
                bool generateMips = true
            );
            static void defaultImageCreateInfo(VkImageCreateInfo &createInfo);
            static void defaultSamplerCreateInfo(SumiDevice &device, VkSamplerCreateInfo &createInfo);

            VkDescriptorImageInfo& getDescriptorInfo() { return descriptorInfo; }

        private:

            void createTextureImage(
                VkMemoryPropertyFlags memoryPropertyFlags, 
                VkImageCreateInfo &imageInfo,
                SumiBuffer &stagingBuffer,
                bool generateMips
            );
            void generateMipChain(VkImageCreateInfo &imageInfo);
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