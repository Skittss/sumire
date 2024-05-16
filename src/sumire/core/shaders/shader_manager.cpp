#include <sumire/core/shaders/shader_manager.hpp>

#include <cassert>

namespace sumire::shaders {

    ShaderManager g_ShaderManager = ShaderManager{};

    ShaderSource* ShaderManager::getShaderSource(std::string shaderPath) {
        assert(sumiDevice != nullptr && "SumiDevice needs setting before modifying any shader maps.");
        if (!shaderMap.sourceExists(shaderPath)) {
            shaderMap.addSource(sumiDevice, shaderPath);
        }

        return shaderMap.getSource(shaderPath);
    }


}