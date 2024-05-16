#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace sumire::shaders {

    class ShaderSource {
    public:
        ShaderSource(SumiDevice* sumiDevice, std::string sourcePath);
        ~ShaderSource() = default;

        void invalidate();
        void revalidate();

    private:
        void initShaderSource(bool hotReload);
        void hotReloadShaderSource();
        std::vector<char> readFile(const std::string& filepath);
        void getSourceParents(const std::vector<char>& shaderCode);
        void compile();
        void createShaderModule(const std::vector<char>& spvCode);

        SumiDevice* sumiDevice = nullptr;
        
        std::string sourcePath;
        std::vector<ShaderSource*> parents;

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        bool invalid = false;
    };

}