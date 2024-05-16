#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/shaders/shader_source.hpp>

#include <unordered_map>

namespace sumire::shaders {

    class ShaderMap {
    public:
        ShaderMap() = default;

        void addSource(SumiDevice* sumiDevice, const std::string& sourcePath);
        bool sourceExists(const std::string& sourcePath);
        ShaderSource* getSource(const std::string& sourcePath);

    private:
        enum SourceType {
            COMPUTE,
            GRAPHICS
            // RAYTRACING
        };

        SourceType getSourceType(const std::string& sourcePath);

        std::unordered_map<std::string, ShaderSource> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;
    };

}