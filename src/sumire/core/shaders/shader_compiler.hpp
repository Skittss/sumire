#pragma once

#include <sumire/core/shaders/shader_source.hpp>

// Shaderc available through vulkan libs.
#include <shaderc/shaderc.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sumire {

    class ShaderSource;

    // ---- Includer ----------------------------------------------------------------------------------------------
    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
    public:
        ShaderIncluder();

        shaderc_include_result* GetInclude(
            const char* requestedSource,
            shaderc_include_type type,
            const char* requestingSource,
            size_t includeDepth
        ) override;

        void ReleaseInclude(shaderc_include_result* result) override;

    private:
        static inline const std::string sEmpty = "";
        static inline shaderc_include_result smFailResult{ 
            sEmpty.c_str(), 0, 
            "Header does not exist!", 0, 
            nullptr
        };

        bool readHeader(const std::string& headerPath, std::vector<char>& buffer);

        std::unordered_map<std::string, std::unique_ptr<shaderc_include_result>> includes{};
    };

    // ---- Compiler ----------------------------------------------------------------------------------------------
    class ShaderCompiler {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        bool compile(const std::string& sourcePath, std::vector<char>& spvOut);

    private:
        static constexpr shaderc_env_version targetVulkanApiVersion = shaderc_env_version_vulkan_1_3;
        static constexpr shaderc_spirv_version targetSpirvVersion = shaderc_spirv_version_1_3;
        static constexpr shaderc_optimization_level optLevel = shaderc_optimization_level_performance;

        bool readSource(const std::string& sourcePath, std::vector<char>& buffer);
        shaderc_shader_kind getShaderType(const std::string& sourcePath);

    };

}