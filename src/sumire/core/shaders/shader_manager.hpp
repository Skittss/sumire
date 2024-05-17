#pragma once

#include <sumire/core/shaders/shader_map.hpp>

#include <string>

namespace sumire {

    class ShaderManager {
    public:
        ShaderManager(VkDevice device);

        ShaderSource* requestShaderSource(std::string shaderPath, SumiPipeline* requester);
        ShaderSource* requestShaderSource(std::string shaderPath, SumiComputePipeline* requester);

    private:
        VkDevice device;

        ShaderMap shaderMap{};
    };

}