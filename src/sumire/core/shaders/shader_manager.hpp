#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/shaders/shader_source.hpp>
#include <sumire/util/sumire_engine_path.hpp>

// File watchers
#include <sumire/core/shaders/shader_update_listener.hpp>
#ifdef _WIN32
#include <sumire/watchers/fs_watcher_win.hpp>
#endif

// Shader compiling
#include <sumire/core/shaders/shader_glslang_compiler.hpp>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include <set>

namespace sumire {

    class SumiPipeline;
    class SumiComputePipeline;

    class ShaderManager {

        friend class ShaderUpdateListener;

    public:
        ShaderManager(VkDevice device, bool hotReloadingEnabled);
        ~ShaderManager();

        ShaderSource* requestShaderSource(std::string shaderPath, SumiPipeline* requester);
        ShaderSource* requestShaderSource(std::string shaderPath, SumiComputePipeline* requester);

    private:
        void addGraphicsSource(
            VkDevice device, const std::string& sourcePath, SumiPipeline* dependency);
        void addGraphicsDependency(const std::string& sourcePath, SumiPipeline* dependency);
        void addComputeSource(
            VkDevice device, const std::string& sourcePath, SumiComputePipeline* dependency);
        void addComputeDependency(const std::string& sourcePath, SumiComputePipeline* dependency);
        void hotReload(const std::string& sourcePath);
        void invalidateSourceAndChildren(const std::string& sourcePath);
        void revalidateSources();
        void updatePipelines();
        void resolveSourceParents(ShaderSource* source, SumiPipeline* dependency);
        void resolveSourceParents(ShaderSource* source, SumiComputePipeline* dependency);
        bool sourceExists(const std::string& sourcePath) const;
        ShaderSource* getSource(const std::string& sourcePath) const;

        std::string formatPath(const std::string& path) const;

        std::unordered_map<std::string, std::unique_ptr<ShaderSource>> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;
        std::set<SumiPipeline*> graphicsPipelinesPendingUpdate;
        std::set<SumiComputePipeline*> computePipelinesPendingUpdate;

        const bool hotReloadingEnabled;

        VkDevice device_;

        // ---- Hot reloading ------------------------------------------------------------------------------------
        std::unique_ptr<ShaderUpdateListener> listener; 
        std::unique_ptr<ShaderGlslangCompiler> compiler;
        void initShaderCompiler();

        static constexpr char* shaderDir = SUMIRE_ENGINE_PATH("shaders/");
        void initShaderDirWatcher();
        void startShaderDirWatcher();
        void stopShaderDirWatcher();
#ifdef _WIN32
        std::unique_ptr<watchers::FsWatcherWin> shaderDirWatcher;
#endif

    };

}