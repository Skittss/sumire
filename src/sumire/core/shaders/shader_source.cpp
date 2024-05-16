#include <sumire/core/shaders/shader_source.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <fstream>

namespace sumire::shaders {

    ShaderSource::ShaderSource(
        SumiDevice* sumiDevice, std::string sourcePath
    ) : sumiDevice{ sumiDevice }, sourcePath { sourcePath } {
        initShaderSource(true);
    }

    void ShaderSource::invalidate() {
        invalid = true;

        for (auto& parent : parents) {
            parent->invalidate();
        }
    }

    void ShaderSource::initShaderSource(bool hotReload) {
        if (hotReload) {
            std::vector<char> shaderCode = readFile(sourcePath);
            getSourceParents(shaderCode);
        }
        std::vector<char> spvCode = readFile(sourcePath + ".spv");
        createShaderModule(spvCode);
    }

    void ShaderSource::hotReloadShaderSource() {
        std::vector<char> newShaderCode = readFile(sourcePath);
        getSourceParents(newShaderCode);
        compile();
        std::vector<char> newSpvCode = readFile(sourcePath + ".spv");
        createShaderModule(newSpvCode);
    }

    std::vector<char> ShaderSource::readFile(const std::string& filepath) {
        std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

        if (!file.is_open()) {
            throw std::runtime_error(
                "[Sumire::ShaderSource] Could not open file: " 
                + filepath + ". (SPIR-V needs to be compiled before startup)."
            );
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void ShaderSource::getSourceParents(const std::vector<char>& shaderCode) {
        // TODO: Read includes and fetch parent ShaderSources (they may not be added yet)
    }

    void ShaderSource::compile() {
        // TODO: Compile with gslang?
    }

    void ShaderSource::createShaderModule(const std::vector<char>& spvCode) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spvCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());

        VK_CHECK_SUCCESS(
            vkCreateShaderModule(sumiDevice->device(), &createInfo, nullptr, &shaderModule),
            "[Sumire::ShaderSource] Failed to create shader module."
        );
    }
}