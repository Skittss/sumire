#pragma once

#include <sumire/core/shaders/shader_map.hpp>

#include <string>

namespace sumire::shaders {

    class ShaderManager {
    public:
        ShaderManager() = default;

        void setDevice(SumiDevice* device) { sumiDevice = device; }

        ShaderSource* getShaderSource(std::string shaderPath);

    private:
        SumiDevice* sumiDevice;

        ShaderMap shaderMap{};
    };

    extern ShaderManager g_ShaderManager;

}