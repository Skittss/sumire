#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>

#define VK_CHECK_SUCCESS(result, error_msg)     \
{                                               \
    VkResult res = (result);                    \
    if (res != VK_SUCCESS) {                    \
        throw std::runtime_error((error_msg));  \
    }                                           \
}