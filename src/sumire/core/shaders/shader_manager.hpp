#pragma once

#include <sumire/core/shaders/shader_map.hpp>

#include <string>

namespace sumire {

    class ShaderManager {
    public:
        ShaderManager(VkDevice device);

        ShaderSource* getShaderSource(std::string shaderPath);

    private:
        VkDevice device;

        ShaderMap shaderMap{};
    };

}