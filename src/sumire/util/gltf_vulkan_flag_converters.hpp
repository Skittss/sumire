#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>

namespace sumire::util {
    
    VkFilter GLTF2VK_FilterMode(int filterMode);
    VkSamplerAddressMode GLTF2VK_SamplerAddressMode(int samplerAddressMode);

}