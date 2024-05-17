#include <sumire/core/shaders/shader_manager.hpp>

namespace sumire {

    ShaderManager::ShaderManager(
        VkDevice device
    ) : device{ device } {}

    ShaderSource* ShaderManager::getShaderSource(std::string shaderPath) {
        if (!shaderMap.sourceExists(shaderPath)) {
            shaderMap.addSource(device, shaderPath);
        }

        return shaderMap.getSource(shaderPath);
    }


}