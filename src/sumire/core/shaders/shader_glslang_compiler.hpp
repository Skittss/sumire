#pragma once

#include <sumire/core/shaders/shader_source.hpp>

// These includes should be available through the vulkan installation.
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sumire {

    class ShaderSource;

    using SourcesMap = std::unordered_map<std::string, std::unique_ptr<ShaderSource>>;

    // ---- Includer ----------------------------------------------------------------------------------------------
    class ShaderGlslangIncluder : public glslang::TShader::Includer {
    public:
        ShaderGlslangIncluder(SourcesMap& sources);

        // System paths checked as a fallback to local
        virtual IncludeResult* includeSystem(
            const char* headerName, const char* includerName, size_t inclusionDepth) override;

        // Local Paths checked first
        virtual IncludeResult* includeLocal(
            const char* headerName, const char* includerName, size_t inclusionDepth) override;

        virtual void releaseInclude(IncludeResult* result) override;

    private:
        std::vector<char> readHeader(const std::string& headerPath);

        SourcesMap& sources;

    };

    // ---- Compiler ----------------------------------------------------------------------------------------------
    class ShaderGlslangCompiler {
    public:
        ShaderGlslangCompiler(SourcesMap& sources);

        void compile(const std::string& sourcePath);

    private:
        static constexpr glslang::EshTargetClientVersion targetVulkanApiVersion =
            glslang::EShTargetVulkan_1_3;
        static constexpr glslang::EShTargetLanguageVersion spirvVersion =
            glslang::EShTargetSpv_1_3;
        static constexpr int defaultGLSLversion = 450;

        std::vector<char> readSource(const std::string& sourcePath);
        EShLanguage getGlslangShaderType(const std::string& sourcePath);

        SourcesMap& sources;
    };

}