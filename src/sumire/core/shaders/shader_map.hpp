//#pragma once
//
//#include <vulkan/vulkan.h>
//
//#include <sumire/core/shaders/shader_source.hpp>
//
//#include <unordered_map>
//#include <memory>
//
//namespace sumire {
//
//    class SumiPipeline;
//    class SumiComputePipeline;
//
//    class ShaderMap {
//    public:
//        ShaderMap(bool hotReloadingEnabled);
//
//        ShaderSource* requestSource(std::string shaderPath, SumiPipeline* requester);
//        ShaderSource* requestShaderSource(std::string shaderPath, SumiComputePipeline* requester);
//
//    private:
//        void addGraphicsSource(
//            VkDevice device, const std::string& sourcePath, SumiPipeline* dependency);
//        void addComputeSource(
//            VkDevice device, const std::string& sourcePath, SumiComputePipeline* dependency);
//        bool sourceExists(const std::string& sourcePath) const;
//        ShaderSource* getSource(const std::string& sourcePath) const;
//
//        std::unordered_map<std::string, std::unique_ptr<ShaderSource>> sources;
//        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
//        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;
//
//        const bool hotReloadingEnabled;
//    };
//
//}