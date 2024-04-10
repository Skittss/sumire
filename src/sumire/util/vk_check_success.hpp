#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>
#include <string>

#define VK_CHECK_SUCCESS(result, error_msg)                                             \
{                                                                                       \
    VkResult res = (result);                                                            \
    if (res != VK_SUCCESS) {                                                            \
        std::string vk_error_msg = string_VkResult(res);                                \
        std::string user_error_msg = (error_msg);                                       \
        throw std::runtime_error(user_error_msg + " [VkResult: " + vk_error_msg + "]"); \
    }                                                                                   \
}
