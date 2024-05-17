#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/shaders/shader_source.hpp>

#include <unordered_map>

namespace sumire {

    class SumiPipeline;
    class SumiComputePipeline;

    class ShaderMap {
    public:
        ShaderMap() = default;

        void addGraphicsSource(
            VkDevice device, const std::string& sourcePath, SumiPipeline* dependency);
        void addComputeSource(
            VkDevice device, const std::string& sourcePath, SumiComputePipeline *dependency);
        bool sourceExists(const std::string& sourcePath);
        ShaderSource* getSource(const std::string& sourcePath);

    private:
        std::unordered_map<std::string, ShaderSource> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;
    };

}