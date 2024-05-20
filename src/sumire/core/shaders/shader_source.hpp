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
        std::vector<ShaderSource*> revalidate();

        enum SourceType {
            GRAPHICS,
            COMPUTE,
            // RAYTRACING
            INCLUDE
        };
        SourceType getSourceType() const { return sourceType; }
        VkShaderModule getShaderModule() const { return shaderModule; }
        std::string getSourcePath() const { return sourcePath; }
        std::vector<std::string> getSourceIncludes();

        void addParent(ShaderSource* parent) { parents.push_back(parent); }
        void addChild(ShaderSource* child) { children.push_back(child); }

    private:
        void initShaderSource();
        void hotReloadShaderSource();
        std::vector<char> readFile(const std::string& filepath);
        void recompile();
        void createShaderModule(const std::vector<char>& spvCode);
        void destroyShaderModule();

        void findSourceType();

        VkDevice device;
        
        std::string sourcePath;
        SourceType sourceType;
        std::vector<ShaderSource*> parents;
        std::vector<ShaderSource*> children;

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        bool invalid = false;
    };

}