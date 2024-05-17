#include <sumire/core/shaders/shader_map.hpp>

#include <iostream>
#include <cassert>

namespace sumire {

    void ShaderMap::addGraphicsSource(
        VkDevice device, 
        const std::string& sourcePath,
        SumiPipeline* dependency
    ) {
        assert(sumiDevice != nullptr && "Cannot add a source with no SumiDevice specified.");

        if (sourceExists(sourcePath)) {
            std::cout << "[Sumire::ShaderMap] WARNING: "
                << "Attempted to add already existing shader source to the shader map. ("
                << sourcePath << "). The existing entry was not modified." << std::endl;
        }
        else {
            ShaderSource newSource = ShaderSource{ device, sourcePath };
            assert(newSource.getSourceType() == ShaderSource::SourceType::COMPUTE
                && "Loaded incompatible shader source type when attempting to add a graphics source.");

            sources.emplace(sourcePath, std::move(newSource));

            // Add dependency
            auto& dependencyEntry = graphicsDependencies.find(sourcePath);
            if (dependencyEntry == graphicsDependencies.end()) {
                graphicsDependencies.emplace(sourcePath, std::vector<SumiPipeline*>{ dependency });
            }
            else {
                dependencyEntry->second.push_back(dependency);
            }
        }
    }

    void ShaderMap::addComputeSource(
        VkDevice device, 
        const std::string& sourcePath,
        SumiComputePipeline* dependency
    ) {
        assert(sumiDevice != nullptr && "Cannot add a source with no SumiDevice specified.");

        if (sourceExists(sourcePath)) {
            std::cout << "[Sumire::ShaderMap] WARNING: "
                << "Attempted to add already existing shader source to the shader map. ("
                << sourcePath << "). The existing entry was not modified." << std::endl;
        }
        else {
            ShaderSource newSource = ShaderSource{ device, sourcePath };
            assert(newSource.getSourceType() == ShaderSource::SourceType::COMPUTE
                && "Loaded incompatible shader source type when attempting to add a compute source.");

            sources.emplace(sourcePath, std::move(newSource));

            // Add dependency
            auto& dependencyEntry = computeDependencies.find(sourcePath);
            if (dependencyEntry == computeDependencies.end()) {
                computeDependencies.emplace(sourcePath, std::vector<SumiComputePipeline*>{ dependency });
            }
            else {
                dependencyEntry->second.push_back(dependency);
            }
        }
    }

    bool ShaderMap::sourceExists(const std::string& sourcePath) {
        return sources.find(sourcePath) != sources.end();
    }

    ShaderSource* ShaderMap::getSource(const std::string& sourcePath) {
        return &sources.at(sourcePath);
    }

}