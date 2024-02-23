#include <sumire/core/sumi_texture.hpp>

#include <stb_image.h>

#include <stdexcept>
#include <cassert>

namespace sumire {

    SumiTexture::SumiTexture(
		SumiDevice &device, 
		VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo,
		SumiBuffer &imageStagingBuffer
	): sumiDevice{ device }, memoryPropertyFlags{ memoryPropertyFlags }
	{
		createTextureImage(memoryPropertyFlags, imageInfo, imageStagingBuffer);
    }

	SumiTexture::~SumiTexture() {
		vkDestroyImage(sumiDevice.device(), image, nullptr);
		vkFreeMemory(sumiDevice.device(), memory, nullptr);
	}

	std::unique_ptr<SumiTexture> SumiTexture::createFromFile(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo, 
		const std::string &filepath
	) {
		int textureWidth, textureHeight, textureChannels;
		stbi_uc *imageData = stbi_load(filepath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (!imageData) 
			throw std::runtime_error("Failed to load image <" + filepath + ">");

		VkDeviceSize imageSize = textureWidth * textureHeight * 4;

		// Upload to GPU memory
		SumiBuffer stagingBuffer{
			device,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)imageData);

		// Cleanup image in system memory from load
		stbi_image_free(imageData);

		return std::make_unique<SumiTexture>(device, memoryPropertyFlags, imageInfo, stagingBuffer);
	}

	// Creates a texture from RGBA data.
	// std::unique_ptr<SumiTexture> SumiTexture::createFromData(
	// 	SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
	// 	vector<glm::.,
	// ) {

	// }

	void SumiTexture::defaultImageCreateInfo(VkImageCreateInfo &createInfo) {
		VkImageCreateInfo imageInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = -1;
		createInfo.extent.height = -1;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.format = VK_FORMAT_R8G8B8_SRGB;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.flags = 0;
	}

	void SumiTexture::createTextureImage(
		VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo, 
		SumiBuffer &stagingBuffer
	) {
		// Ensure image dimensions are set
		assert(imageInfo.extent.width > 0 && imageInfo.extent.height > 0);
		// Do not allow image creation if already created
		assert(image || memory);

		sumiDevice.createImageWithInfo(imageInfo, memoryPropertyFlags, image, memory);
		
		// Image to TRANSFER_DST_OPTIMAL for buffer copying
		sumiDevice.transitionImageLayout(
			image, 
			imageInfo.format, 
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);
		// Copy image from staging buffer to GPU handle
		sumiDevice.copyBufferToImage(stagingBuffer.getBuffer(), image, imageInfo.extent.height, imageInfo.extent.height, 1);
		
		// Image to shader compatible layout
		sumiDevice.transitionImageLayout(
			image,
			imageInfo.format, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
}