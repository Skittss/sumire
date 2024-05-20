#include <sumire/core/shaders/shader_source.hpp>
#include <sumire/util/vk_check_success.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>

namespace sumire {

    ShaderSource::ShaderSource(
        VkDevice device, std::string sourcePath
    ) : device{ device }, sourcePath { sourcePath } {
        findSourceType();
        initShaderSource();
    }

    ShaderSource::~ShaderSource() {
        destroyShaderModule();
    }

    void ShaderSource::invalidate() {
        invalid = true;

        for (auto& child : children) {
            child->invalidate();
        }
    }

    void ShaderSource::revalidate() {
        // Ensure all parents are validated before recompiling
        for (auto& parent : parents) {
            parent->revalidate();
        }

        // Includes do not need compiling
        if (invalid && sourceType != SourceType::INCLUDE) {
            recompile();
        }

        invalid = false;
    }

    void ShaderSource::initShaderSource() {
        if (sourceType != SourceType::INCLUDE) {
            std::vector<char> spvCode = readFile(sourcePath + ".spv");
            createShaderModule(spvCode);
        }
    }

    void ShaderSource::hotReloadShaderSource() {
        if (sourceType != SourceType::INCLUDE) {
            recompile();
            std::vector<char> newSpvCode = readFile(sourcePath + ".spv");
            createShaderModule(newSpvCode);
        }
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

    std::vector<std::string> ShaderSource::getSourceIncludes() {
        std::ifstream file{ sourcePath, std::ios::in };

        file.seekg(0);

        if (!file.is_open()) {
            throw std::runtime_error(
                "[Sumire::ShaderSource] Could not open source file: "
                + sourcePath
            );
        }
        
        // Note: There are whitespace characters not considered as I don't think
        //       these should pass the glsl compiler - \f \v
        std::vector<std::string> includes{};
        std::string line;
        while (std::getline(file, line)) {
            size_t firstNonWsp = line.find_first_not_of(" \t\r\n");
            if (firstNonWsp == line.npos) continue;

            // #include
            int includeCmp = line.compare(firstNonWsp, 8, "#include");
            if (includeCmp != 0) continue;

            // At least one whitespace following #include
            size_t incWspIdx = firstNonWsp + 8;
            char& incWsp = line.at(incWspIdx);
            if ( !(incWsp == ' ' || incWsp == '\t') ) continue;

            // Next non wsp character must be open quote: "
            size_t openQuoteIdx = line.find_first_not_of(" \t\r\n", incWspIdx + 1);
            if (openQuoteIdx == line.npos || line.at(openQuoteIdx) != '"') continue;

            // Must be a close quote to finish the include;
            size_t closeQuoteIdx = line.find_first_of('"', openQuoteIdx + 1);
            if (closeQuoteIdx == line.npos) continue;

            // Only allow trailing whitespace after close quote
            if (line.find_first_not_of(" \t\r\n", closeQuoteIdx + 1) != line.npos) continue;

            std::string includePath = line.substr(openQuoteIdx + 1, closeQuoteIdx - openQuoteIdx - 1);
            includes.push_back(includePath);
        }

        file.close();

        return includes;
    }

    void ShaderSource::recompile() {
        std::cout << "[Sumire::ShaderSource] Recompiling Shader Source: " << sourcePath << std::endl;

        // TODO: Compile with gslang?

        // Recreate shader module
    }

    void ShaderSource::createShaderModule(const std::vector<char>& spvCode) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spvCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());

        VK_CHECK_SUCCESS(
            vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
            "[Sumire::ShaderSource] Failed to create shader module."
        );
    }

    void ShaderSource::destroyShaderModule() {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    void ShaderSource::findSourceType() {
        std::filesystem::path fp = sourcePath;
        std::filesystem::path ext = fp.extension();

        if (ext == ".frag" || ext == ".vert") {
            sourceType = SourceType::GRAPHICS;
        }
        else if (ext == ".comp") {
            sourceType = SourceType::COMPUTE;
        }
        else if (ext == ".glsl") {
            sourceType = SourceType::INCLUDE;
        }
        else {
            throw std::runtime_error(
                "[Sumire::ShaderSource] Incompatible shader extension present for shader " 
                + sourcePath
            );
        }
    }
}