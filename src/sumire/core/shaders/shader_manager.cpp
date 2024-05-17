#include <sumire/core/shaders/shader_manager.hpp>

namespace sumire {

    ShaderManager::ShaderManager(
        VkDevice device
    ) : device{ device } {}

    ShaderSource* ShaderManager::requestShaderSource(
        std::string shaderPath, SumiPipeline* requester
    ) {
        if (!shaderMap.sourceExists(shaderPath)) {
            shaderMap.addGraphicsSource(device, shaderPath, requester);
        }

        return shaderMap.getSource(shaderPath);
    }

    ShaderSource* ShaderManager::requestShaderSource(
        std::string shaderPath, SumiComputePipeline* requester
    ) {
        if (!shaderMap.sourceExists(shaderPath)) {
            shaderMap.addComputeSource(device, shaderPath, requester);
        }

        return shaderMap.getSource(shaderPath);
    }


}