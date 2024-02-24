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
		createTextureImageView(imageInfo.format);
		createTextureSampler();
    }

	SumiTexture::~SumiTexture() {
		vkDestroySampler(sumiDevice.device(), sampler, nullptr);
		vkDestroyImageView(sumiDevice.device(), imageView, nullptr);
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
		imageInfo.extent.width = textureWidth;
		imageInfo.extent.height = textureHeight;

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

	// Creates a RGBA texture from RGBA data.
	std::unique_ptr<SumiTexture> SumiTexture::createFromRGBA(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
		int width, int height, unsigned char *data
	) {
		VkDeviceSize imageSize = width * height * 4;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;

		// Upload to GPU memory
		SumiBuffer stagingBuffer{
			device,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)data);

		return std::make_unique<SumiTexture>(device, memoryPropertyFlags, imageInfo, stagingBuffer);
	}

	// Creates a RGBA texture from RGB data.
	std::unique_ptr<SumiTexture> SumiTexture::createFromRGB(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, VkImageCreateInfo &imageInfo,
		int width, int height, unsigned char *data
	) {		
		VkDeviceSize imageSize = width * height * 4;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;

		// Convert RGB to RGBA (RGB is generally less supported)
		unsigned char *convertedData = new unsigned char[imageSize];
		unsigned char *rgbaPtr = convertedData;
		unsigned char *rgbPtr = data;

		for (int i = 0; i < width * height; i++) {
			for (int j = 0; j < 3; j++) {
				rgbaPtr[j] = rgbPtr[j];
			}
			rgbaPtr += 4;
			rgbPtr += 3;
		}

		// Upload to GPU memory
		SumiBuffer stagingBuffer{
			device,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)convertedData);
		delete[] convertedData;

		return std::make_unique<SumiTexture>(device, memoryPropertyFlags, imageInfo, stagingBuffer);
	}

	void SumiTexture::defaultImageCreateInfo(VkImageCreateInfo &createInfo) {
		VkImageCreateInfo imageInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = -1;
		createInfo.extent.height = -1;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
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
		assert(imageInfo.extent.width > 0 && imageInfo.extent.height > 0 && "Texture image dimensions not set");
		// Do not allow image creation if already created
		assert(!(image || memory) && "Image already created for texture");

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

	void SumiTexture::createTextureImageView(VkFormat format) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(sumiDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture image view");
		}
	}

	void SumiTexture::createTextureSampler() {
		// TODO: It would probably be worth querying these *once* at program start, 
		//       and creating a const table which can be imported instead of having to re-query.
		// Query support for certain sampling features (e.g. anisotropy)
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(sumiDevice.getPhysicalDevice(), &deviceProperties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(sumiDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture sampler");
		}
	}
}