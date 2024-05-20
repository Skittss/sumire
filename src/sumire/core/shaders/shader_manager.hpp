#pragma once

#include <vulkan/vulkan.h>

#include <sumire/core/shaders/shader_source.hpp>
#include <sumire/util/sumire_engine_path.hpp>

// File watchers
#include <sumire/watchers/fs_watch_listener.hpp>
#ifdef _WIN32
#include <sumire/watchers/fs_watcher_win.hpp>
#endif

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>

namespace sumire {

    class SumiPipeline;
    class SumiComputePipeline;

    class ShaderManager {
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
        void resolveSourceParents(ShaderSource* source, SumiPipeline* dependency);
        void resolveSourceParents(ShaderSource* source, SumiComputePipeline* dependency);
        bool sourceExists(const std::string& sourcePath) const;
        ShaderSource* getSource(const std::string& sourcePath) const;

        std::string formatPath(const std::string& path) const;

        std::unordered_map<std::string, std::unique_ptr<ShaderSource>> sources;
        std::unordered_map<std::string, std::vector<SumiPipeline*>> graphicsDependencies;
        std::unordered_map<std::string, std::vector<SumiComputePipeline*>> computeDependencies;

        const bool hotReloadingEnabled;

        VkDevice device_;

        // ---- Hot reloading ------------------------------------------------------------------------------------
        class ShaderUpdateListener : public watchers::FsWatchListener {
        public:
            void handleFileAction(
                watchers::FsWatchAction action,
                const std::string& dir,
                const std::string& filename
            ) override;
        };
        std::unique_ptr<ShaderUpdateListener> listener; 

        static constexpr char* shaderDir = SUMIRE_ENGINE_PATH("shaders/");
        void initShaderDirWatcher();
        void startShaderDirWatcher();
        void stopShaderDirWatcher();
#ifdef _WIN32
        std::unique_ptr<watchers::FsWatcherWin> shaderDirWatcher;
#endif

    };

}