#include <sumire/util/gltf_vulkan_flag_converters.hpp>

namespace sumire::util {

    // Convert GLTF filtering flags to corresponding Vulkan flags.
    VkFilter GLTF2VK_FilterMode(int filterMode) {
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-sampler

        switch (filterMode) {
            case -1:
            case 9728:
                return VK_FILTER_NEAREST;
            case 9729:
                return VK_FILTER_LINEAR;
            case 9984:
                return VK_FILTER_NEAREST;
            case 9985:
                return VK_FILTER_NEAREST;
            case 9986:
                return VK_FILTER_LINEAR;
            case 9987: 
                return VK_FILTER_LINEAR;
        }

        std::invalid_argument("Invalid GLTF filter mode attempted Vulkan conversion: <" + std::to_string(filterMode) + ">");
        return VK_FILTER_NEAREST; // return to prevent compiler warning
    }

    // Convert GLTF sampler addressing flags to corresponding Vulkan flags.
    VkSamplerAddressMode GLTF2VK_SamplerAddressMode(int samplerAddressMode) {
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-sampler

        switch (samplerAddressMode) {
            case -1:
            case 10497:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case 33071:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case 33648:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }

        std::invalid_argument("Invalid GLTF sampler address (wrap) mode attempted Vulkan conversion: <" + std::to_string(samplerAddressMode) + ">");
        return VK_SAMPLER_ADDRESS_MODE_REPEAT; // return to prevent compiler warning
    }

}