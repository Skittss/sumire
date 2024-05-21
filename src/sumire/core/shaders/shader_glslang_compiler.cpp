#include <sumire/core/shaders/shader_glslang_compiler.hpp>

#include <sumire/util/read_file_binary.hpp>

#include <glslang/Public/ResourceLimits.h>

#include <filesystem>
#include <array>
#include <iostream>

namespace sumire {

    // ---- Includer --------------------------------------------------------------------------------------------
    using TShaderIncluder = glslang::TShader::Includer;

    ShaderGlslangIncluder::ShaderGlslangIncluder(
        SourcesMap& sources
    ) : sources{ sources } {}

    TShaderIncluder::IncludeResult* ShaderGlslangIncluder::includeSystem(
        const char* headerName, const char* includerName, size_t inclusionDepth
    ) {
        return nullptr;
    }

    TShaderIncluder::IncludeResult* ShaderGlslangIncluder::includeLocal(
        const char* headerName, const char* includerName, size_t inclusionDepth
    ) {
        std::filesystem::path headerPath{ headerName };
        std::filesystem::path headerAbsPath = std::filesystem::absolute(headerPath);

        std::string resolvedHeaderName = headerAbsPath.u8string();

        // TODO: consider caching this
        std::vector<char> headerSource = readHeader(resolvedHeaderName);

        auto result = std::make_unique<TShaderIncluder::IncludeResult>(
            resolvedHeaderName, 
            headerSource.data(),
            headerSource.size(),
            nullptr
        );

        return result.get();
    }

    void ShaderGlslangIncluder::releaseInclude(TShaderIncluder::IncludeResult* result) {

    }

    std::vector<char> ShaderGlslangIncluder::readHeader(const std::string& headerPath) {
        std::vector<char> buffer;
        bool success = util::readFileBinary(headerPath, buffer);

        if (!success) {
            throw std::runtime_error(
                "[Sumire::ShaderGlslangCompiler] Could not open header: "
                + headerPath
            );
        }

        return buffer;
    }

    // ---- Compiler --------------------------------------------------------------------------------------------
    ShaderGlslangCompiler::ShaderGlslangCompiler(
        SourcesMap& sources
    ) : sources{ sources } {}

    void ShaderGlslangCompiler::compile(const std::string& sourcePath) {
        std::vector<char> sourceBuffer = readSource(sourcePath);
        EShLanguage shaderLang         = getGlslangShaderType(sourcePath);
        glslang::TShader shader{ shaderLang };

        std::array<const char*, 1> sourceStrings{ sourceBuffer.data() };
        shader.setStrings(sourceStrings.data(), 1);

        shader.setEnvClient(glslang::EShClientVulkan, targetVulkanApiVersion);
        shader.setEnvTarget(glslang::EshTargetSpv, spirvVersion);
        shader.setEntryPoint("main");

        const TBuiltInResource* resources = GetDefaultResources();
        const bool forwardCompatible      = false;
        const EShMessages messageFlags    = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
        EProfile defaultProfile           = ENoProfile;

        ShaderGlslangIncluder includer{ sources };

        // Preprocess
        std::string preprocessedSource;
        if (!shader.preprocess(
            resources,
            defaultGLSLversion,
            defaultProfile,
            false,
            forwardCompatible,
            messageFlags,
            &preprocessedSource,
            includer
        )) {
            std::cout << "[Sumire::ShaderGlslangCompiler] Failed to preprocess shader "
                << sourcePath << ":\n\n" << shader.getInfoLog() << std::endl;
        }

        std::array<const char*, 1> preprocessedSources{ preprocessedSource.c_str() };
        shader.setStrings(preprocessedSources.data(), 1);

        // Parse
        if (!shader.parse(
            resources,
            defaultGLSLversion,
            defaultProfile,
            false,
            forwardCompatible,
            messageFlags,
            includer
        )) {
            std::cout << "[Sumire::ShaderGlslangCompiler] Failed to parse shader"
                << sourcePath << ":\n\n" << shader.getInfoLog() << std::endl;
        }

        // Link
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messageFlags)) {
            std::cout << "[Sumire::ShaderGlslangCompiler] Failed to link shader"
                << sourcePath << ":\n\n" << shader.getInfoLog() << std::endl;
        }

        // AST -> SPV
        glslang::TIntermediate& intermediateRef = *(program.getIntermediate(shaderLang));
        std::vector<uint32_t> spirv;
        glslang::SpvOptions options{};
        options.validate = true;
        // TODO: May need to allow debug info here for Debug builds.

        glslang::GlslangToSpv(intermediateRef, spirv, &options);

    }

    std::vector<char> ShaderGlslangCompiler::readSource(const std::string& sourcePath) {
        std::vector<char> buffer;
        bool success = util::readFileBinary(sourcePath, buffer);

        if (!success) {
            throw std::runtime_error(
                "[Sumire::ShaderGlslangCompiler] Could not open shader source for compilation: "
                + sourcePath
            );
        }

        return buffer;
    }

    EShLanguage ShaderGlslangCompiler::getGlslangShaderType(const std::string& sourcePath) {
        std::filesystem::path source{ sourcePath };
        const std::string& ext = source.extension().u8string();

        if      (ext == ".vert") return EShLangVertex;
        else if (ext == ".frag") return EShLangFragment;
        else if (ext == ".comp") return EShLangCompute;
        else if (ext == ".glsl") {
            throw std::runtime_error(
                "[Sumire::ShaderGlslangCompiler] Ambiguous shader type (.glsl) "
                "of shader pending compilation: " + sourcePath
            );
        }
        else {
            throw std::runtime_error(
                "[Sumire::ShaderGlslangCompiler] Unrecognised shader extension "
                "of shader pending compilation: " + sourcePath
            );
        }

        // Default, should never run.
        return EShLangVertex;
    }

}