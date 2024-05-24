#include <sumire/core/shaders/shader_compiler.hpp>

#include <sumire/util/read_file_binary.hpp>
#include <sumire/util/relative_engine_filepath.hpp>

#include <filesystem>
#include <array>
#include <iostream>

namespace sumire {

    // ---- Includer --------------------------------------------------------------------------------------------
    ShaderIncluder::ShaderIncluder() {}

    shaderc_include_result* ShaderIncluder::GetInclude(
        const char* requestedSource, 
        shaderc_include_type type, 
        const char* requestingSource, 
        size_t inclusionDepth
    ) {
        std::string resolvedHeaderName = 
            util::relativeEngineFilepath(requestingSource, requestedSource);

        // Cache include results
        auto includeEntry = includes.find(resolvedHeaderName);
        if (includeEntry != includes.end()) {
            return includeEntry->second.get();
        }

        std::vector<char> headerSource;
        bool success = readHeader(resolvedHeaderName, headerSource);
        if (!success) {
            return &smFailResult;
        }

        auto container = new std::array<std::string, 2>;
        (*container)[0] = resolvedHeaderName;
        (*container)[1] = std::string{ headerSource.begin(), headerSource.end() };

        auto result = std::make_unique<shaderc_include_result>();
        result->user_data          = container;
        result->source_name        = (*container)[0].data();
        result->source_name_length = (*container)[0].size();
        result->content_length     = (*container)[1].size();
        result->content            = (*container)[1].data();

        includes.emplace(resolvedHeaderName, std::move(result));

        return includes.at(resolvedHeaderName).get();
    }

    void ShaderIncluder::ReleaseInclude(shaderc_include_result* result) {
        delete static_cast<std::array<std::string, 2>*>(result->user_data);
        std::string headerName{ result->source_name };

        auto includesEntry = includes.find(headerName);
        if (includesEntry != includes.end()) includes.erase(includesEntry);
    }

    bool ShaderIncluder::readHeader(const std::string& headerPath, std::vector<char>& buffer) {
        return util::readFileBinary(headerPath, buffer);
    }

    // ---- Compiler --------------------------------------------------------------------------------------------
    ShaderCompiler::ShaderCompiler() {}
    ShaderCompiler::~ShaderCompiler() {}

    void ShaderCompiler::compile(const std::string& sourcePath) {
        std::vector<char> sourceBuffer = readSource(sourcePath);
        const std::string sourceCode{ sourceBuffer.begin(), sourceBuffer.end() };

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetTargetEnvironment(shaderc_target_env_vulkan, targetVulkanApiVersion);
        options.SetTargetSpirv(targetSpirvVersion);
        options.SetIncluder(std::make_unique<ShaderIncluder>());
        options.SetOptimizationLevel(optLevel);

        // Preprocess
        const shaderc_shader_kind shaderType = getShaderType(sourcePath);
        auto preprocessResult = compiler.PreprocessGlsl(sourceCode, shaderType, sourcePath.c_str(), options);
        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cout << "[Sumire::ShaderCompiler] Failed to preprocess shader "
                << sourcePath << ":\n\n" << preprocessResult.GetErrorMessage() << std::endl;
            return;
        }

        // Compile
        const std::string preprocessedCode{ preprocessResult.begin(), preprocessResult.end() };
        auto compileResult = compiler.CompileGlslToSpv(preprocessedCode, shaderType, sourcePath.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cout << "[Sumire::ShaderCompiler] Failed to compile shader "
                << sourcePath << ":\n\n" << compileResult.GetErrorMessage() << std::endl;
            return;
        }

        // Write SPIRV
    }

    std::vector<char> ShaderCompiler::readSource(const std::string& sourcePath) {
        std::vector<char> buffer;
        bool success = util::readFileBinary(sourcePath, buffer);

        if (!success) {
            throw std::runtime_error(
                "[Sumire::ShaderCompiler] Could not open shader source for compilation: "
                + sourcePath
            );
        }

        return buffer;
    }

    shaderc_shader_kind ShaderCompiler::getShaderType(const std::string& sourcePath) {
        std::filesystem::path source{ sourcePath };
        const std::string& ext = source.extension().u8string();

        if      (ext == ".vert") return shaderc_vertex_shader;
        else if (ext == ".frag") return shaderc_fragment_shader;
        else if (ext == ".comp") return shaderc_compute_shader;
        else if (ext == ".glsl") {
            throw std::runtime_error(
                "[Sumire::ShaderCompiler] Ambiguous shader type (.glsl) "
                "of shader pending compilation: " + sourcePath
            );
        }
        else {
            throw std::runtime_error(
                "[Sumire::ShaderCompiler] Unrecognised shader extension "
                "of shader pending compilation: " + sourcePath
            );
        }

        // Default, should never run.
        return shaderc_vertex_shader;
    }

}