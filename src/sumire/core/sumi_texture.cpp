#include <sumire/core/sumi_texture.hpp>

#include <stb_image.h>

#include <stdexcept>
#include <cassert>
#include <math.h>

namespace sumire {

    SumiTexture::SumiTexture(
		SumiDevice &device, 
		VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo,
		bool generateMips,
		VkSamplerCreateInfo &samplerInfo,
		SumiBuffer &imageStagingBuffer
	): sumiDevice{ device }, memoryPropertyFlags{ memoryPropertyFlags }
	{
		if (generateMips) {
			// Check that linear filtering is supported for mipmap generation (using VkCmdBlitImage)
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(sumiDevice.getPhysicalDevice(), imageInfo.format, &formatProperties);
		
			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
				throw std::runtime_error("Could not generate mip map textures: Linear bitting (for filtering) is not supported by the device used.");
			}
		}

		createTextureImage(memoryPropertyFlags, imageInfo, imageStagingBuffer, generateMips);
		createTextureImageView(imageInfo.format);
		createTextureSampler(samplerInfo);
		writeDescriptorInfo();
    }

	SumiTexture::~SumiTexture() {
		vkDestroySampler(sumiDevice.device(), sampler, nullptr);
		vkDestroyImageView(sumiDevice.device(), imageView, nullptr);
		vkDestroyImage(sumiDevice.device(), image, nullptr);
		vkFreeMemory(sumiDevice.device(), memory, nullptr);
	}

	std::unique_ptr<SumiTexture> SumiTexture::createFromFile(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
		const std::string &filepath,
		bool generateMips
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

		return std::make_unique<SumiTexture>(
			device,
			memoryPropertyFlags, 
			imageInfo, 
			generateMips,
			samplerInfo,
			stagingBuffer
		);
	}

	// Creates a RGBA texture from RGBA data.
	std::unique_ptr<SumiTexture> SumiTexture::createFromRGBA(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
		uint32_t width, uint32_t height, unsigned char *data,
		bool generateMips
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

		return std::make_unique<SumiTexture>(
			device,
			memoryPropertyFlags, 
			imageInfo, 
			generateMips,
			samplerInfo, 
			stagingBuffer
		);
	}

	// Creates a RGBA texture from RGB data.
	std::unique_ptr<SumiTexture> SumiTexture::createFromRGB(
		SumiDevice &device, VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo, VkSamplerCreateInfo &samplerInfo,
		uint32_t width, uint32_t height, unsigned char *data,
		bool generateMips
	) {		
		VkDeviceSize imageSize = width * height * 4;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;

		// Convert RGB to RGBA (RGB is generally less supported)
		unsigned char *convertedData = new unsigned char[imageSize];
		unsigned char *rgbaPtr = convertedData;
		unsigned char *rgbPtr = data;

		for (uint32_t i = 0; i < width * height; i++) {
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

		return std::make_unique<SumiTexture>(
			device, 
			memoryPropertyFlags, 
			imageInfo, 
			generateMips,
			samplerInfo, 
			stagingBuffer
		);
	}

	// Image create info for a linear RGBA (UNORM) image.
	void SumiTexture::defaultImageCreateInfo(VkImageCreateInfo &createInfo) {
		VkImageCreateInfo imageInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = -1;
		createInfo.extent.height = -1;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.flags = 0;
	}

	void SumiTexture::defaultSamplerCreateInfo(SumiDevice &device, VkSamplerCreateInfo &createInfo) {
		// TODO: It would probably be worth querying these *once* at program start, 
		//       and creating a const table which can be imported instead of having to re-query.
		// Query support for certain sampling features (e.g. anisotropy)
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &deviceProperties);

		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;
	}

	void SumiTexture::createTextureImage(
		VkMemoryPropertyFlags memoryPropertyFlags, 
		VkImageCreateInfo &imageInfo, 
		SumiBuffer &stagingBuffer,
		bool generateMips
	) {
		// Ensure image dimensions are set
		assert(imageInfo.extent.width > 0 && imageInfo.extent.height > 0 && "Texture image dimensions not set");
		// Do not allow image creation if already created
		assert(!(image || memory) && "Image already created for texture");

		// Ensure correct bit flags are set for mip mapping and transfer to shader format
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (generateMips) {
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			// Also set number of required mip map levels based on max image extent. 
			imageInfo.mipLevels = static_cast<uint32_t>(
				floor(log2(std::max(imageInfo.extent.width, imageInfo.extent.height))) + 1.0f);
			this->mipLevels = imageInfo.mipLevels; // store for later use in sampler creation.
		}

		sumiDevice.createImageWithInfo(imageInfo, memoryPropertyFlags, image, memory);

		// Image to TRANSFER_DST_OPTIMAL for buffer copying
		sumiDevice.transitionImageLayout(
			image, 
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);
		// Copy image from staging buffer to GPU handle
		sumiDevice.copyBufferToImage(stagingBuffer.getBuffer(), image, imageInfo.extent.height, imageInfo.extent.height, 1);

		// Generate Mip map if needed
		if (generateMips) {
			generateMipChain(imageInfo);
		}
		else {
			// if not generating mip maps, need to manually transition top level mip back to
			//  TRANSFER_SRC for the shader read transition below. (Mip map generator does this transition if called)
			sumiDevice.transitionImageLayout(
				image,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			);
		}
		
		// Image to shader compatible layout (all mip levels)
		sumiDevice.transitionImageLayout(
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, imageInfo.mipLevels , 0, 1 },
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	void SumiTexture::generateMipChain(VkImageCreateInfo &imageInfo) {
		if (imageInfo.mipLevels <= 1) return;
		assert(image != VK_NULL_HANDLE && "Attempted to generate texture mips before image was created");

		// TODO: Could this be done it a separate VkQueue to the graphics rendering queue?

		// Prepare layout of base mip level for copy (from mipMap 0 -> 1)
		sumiDevice.transitionImageLayout(
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);

		// Generate via image blit to make use of GPU image downsampling
		VkCommandBuffer commandBuffer = sumiDevice.beginSingleTimeCommands();

		// Ping pong type behaviour between layers for generating subsequent mip map levels
		// https://docs.vulkan.org/samples/latest/samples/api/texture_mipmap_generation/README.html#_generating_the_mip_chain
		for (uint32_t i = 1; i < imageInfo.mipLevels; i++) {
			VkImageBlit imageBlit{};

			// Src
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel   = i - 1;
			imageBlit.srcOffsets[1].x           = int32_t(imageInfo.extent.width  >> (i - 1));
			imageBlit.srcOffsets[1].y           = int32_t(imageInfo.extent.height >> (i - 1));
			imageBlit.srcOffsets[1].z           = 1;

			// Dst
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel   = i;
			imageBlit.dstOffsets[1].x           = int32_t(imageInfo.extent.width  >> i);
			imageBlit.dstOffsets[1].y           = int32_t(imageInfo.extent.height >> i);
			imageBlit.dstOffsets[1].z           = 1;

			// Make current mip map level able to be transferred to
			sumiDevice.transitionImageLayout(
				image,
				{ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 },
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				commandBuffer
			);

			vkCmdBlitImage(
				commandBuffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				VK_FILTER_LINEAR
			);

			// Prepare the mip map level written just now for transfer to the subsequent mip map level
			sumiDevice.transitionImageLayout(
				image,
				{ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 },
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				commandBuffer
			);
		}

		sumiDevice.endSingleTimeCommands(commandBuffer);
	}

	void SumiTexture::createTextureImageView(VkFormat format) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(sumiDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture image view");
		}
	}

	void SumiTexture::createTextureSampler(VkSamplerCreateInfo &samplerInfo) {
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(sumiDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture sampler");
		}
	}

	void SumiTexture::writeDescriptorInfo() {
		descriptorInfo = VkDescriptorImageInfo{};
		descriptorInfo.sampler = sampler;
		descriptorInfo.imageView = imageView;
		descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}