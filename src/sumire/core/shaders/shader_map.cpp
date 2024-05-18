//#include <sumire/core/shaders/shader_map.hpp>
//
//#include <iostream>
//#include <cassert>
//
//namespace sumire {
//
//    ShaderMap::ShaderMap(bool hotReloadingEnabled) 
//        : hotReloadingEnabled{ hotReloadingEnabled } {}
//
//    ShaderSource* ShaderMap::requestShaderSource(
//        std::string shaderPath, SumiPipeline* requester
//    ) {
//        if (!sourceExists(shaderPath)) {
//            addGraphicsSource(device, shaderPath, requester);
//        }
//
//        return getSource(shaderPath);
//    }
//
//    ShaderSource* ShaderMap::requestShaderSource(
//        std::string shaderPath, SumiComputePipeline* requester
//    ) {
//        if (!shaderMap.sourceExists(shaderPath)) {
//            shaderMap.addComputeSource(device, shaderPath, requester);
//        }
//
//        return shaderMap.getSource(shaderPath);
//    }
//
//
//    void ShaderMap::addGraphicsSource(
//        VkDevice device, 
//        const std::string& sourcePath,
//        SumiPipeline* dependency
//    ) {
//        if (sourceExists(sourcePath)) {
//            std::cout << "[Sumire::ShaderMap] WARNING: "
//                << "Attempted to add already existing shader source to the shader map. ("
//                << sourcePath << "). The existing entry was not modified." << std::endl;
//        }
//        else {
//            std::unique_ptr<ShaderSource> newSource = std::make_unique<ShaderSource>(device, sourcePath);
//            assert(newSource->getSourceType() == ShaderSource::SourceType::GRAPHICS
//                && "Loaded incompatible shader source type when attempting to add a graphics source.");
//
//            // Resolve source parents if hot reloading enabled
//            if (hotReloadingEnabled) {
//                newSource->getSourceIncludes();
//            }
//
//            sources.emplace(sourcePath, std::move(newSource));
//
//            // Add dependency
//            auto& dependencyEntry = graphicsDependencies.find(sourcePath);
//            if (dependencyEntry == graphicsDependencies.end()) {
//                graphicsDependencies.emplace(sourcePath, std::vector<SumiPipeline*>{ dependency });
//            }
//            else {
//                dependencyEntry->second.push_back(dependency);
//            }
//        }
//    }
//
//    void ShaderMap::addComputeSource(
//        VkDevice device, 
//        const std::string& sourcePath,
//        SumiComputePipeline* dependency
//    ) {
//        if (sourceExists(sourcePath)) {
//            std::cout << "[Sumire::ShaderMap] WARNING: "
//                << "Attempted to add already existing shader source to the shader map. ("
//                << sourcePath << "). The existing entry was not modified." << std::endl;
//        }
//        else {
//            std::unique_ptr<ShaderSource> newSource = std::make_unique<ShaderSource>(device, sourcePath);
//            assert(newSource->getSourceType() == ShaderSource::SourceType::COMPUTE
//                && "Loaded incompatible shader source type when attempting to add a compute source.");
//
//            sources.emplace(sourcePath, std::move(newSource));
//
//            // Add dependency
//            auto& dependencyEntry = computeDependencies.find(sourcePath);
//            if (dependencyEntry == computeDependencies.end()) {
//                computeDependencies.emplace(sourcePath, std::vector<SumiComputePipeline*>{ dependency });
//            }
//            else {
//                dependencyEntry->second.push_back(dependency);
//            }
//        }
//    }
//
//    bool ShaderMap::sourceExists(const std::string& sourcePath) const {
//        return sources.find(sourcePath) != sources.end();
//    }
//
//    ShaderSource* ShaderMap::getSource(const std::string& sourcePath) const {
//        return sources.at(sourcePath).get();
//    }
//
//}