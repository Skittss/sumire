#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/shaders/shader_source.hpp>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>

namespace sumire {

    class SumiPipeline;
    class SumiComputePipeline;

    class ShaderManager {
    public:
        ShaderManager(VkDevice device, bool hotReloadingEnabled);

        ShaderSource* requestShaderSource(std::string shaderPath, SumiPipeline* requester);
        ShaderSource* requestShaderSource(std::string shaderPath, SumiComputePipeline* requester);

    private:
        void addGraphicsSource(
            VkDevice device, const std::string& sourcePath, SumiPipeline* dependency);
        void addGraphicsDependency(const std::string& sourcePath, SumiPipeline* dependency);
        void addComputeSource(
            VkDevice device, const std::string& sourcePath, SumiComputePipeline* dependency);
        void addComputeDependency(const std::string& sourcePath, SumiComputePipeline* dependency);
        void resolveSourceParents(ShaderSource* source, SumiPipeline* dependency);
        void resolveSourceParents(ShaderSource* source, SumiComputePipeline* dependency);
        bool sourceExists(const std::string& sourcePath) const;
        ShaderSource* getSource(const std::string& sourcePath) const;

        std::string formatPath(const std::string& path) const;

        std::unordered_map<std::string, std::unique_ptr<ShaderSource>> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;

        const bool hotReloadingEnabled;

        VkDevice device_;

    };

}