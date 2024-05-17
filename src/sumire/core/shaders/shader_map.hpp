#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/shaders/shader_source.hpp>

#include <unordered_map>
#include <memory>

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
        bool sourceExists(const std::string& sourcePath) const;
        ShaderSource* getSource(const std::string& sourcePath) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<ShaderSource>> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;
    };

}