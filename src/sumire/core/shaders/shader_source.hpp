#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace sumire {

    class ShaderSource {
    public:
        ShaderSource(VkDevice device, std::string sourcePath);
        ~ShaderSource();

        void invalidate();
        void revalidate();

        enum SourceType {
            GRAPHICS,
            COMPUTE,
            // RAYTRACING
            INCLUDE
        };
        SourceType getSourceType() const { return sourceType; }

        VkShaderModule getShaderModule() const { return shaderModule; }

    private:
        void initShaderSource(bool hotReload);
        void hotReloadShaderSource();
        std::vector<char> readFile(const std::string& filepath);
        void getSourceParents(const std::vector<char>& shaderCode);
        void compile();
        void createShaderModule(const std::vector<char>& spvCode);
        void destroyShaderModule();

        void findSourceType();

        VkDevice device;
        
        std::string sourcePath;
        SourceType sourceType;
        std::vector<ShaderSource*> parents;

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        bool invalid = false;
    };

}