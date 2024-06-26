#include <sumire/core/shaders/shader_manager.hpp>

#include <sumire/util/relative_engine_filepath.hpp>

#include <iostream>
#include <cassert>

namespace sumire {


    ShaderManager::ShaderManager(
        VkDevice device, bool hotReloadingEnabled
    ) : device_{ device }, hotReloadingEnabled{ hotReloadingEnabled } {
        if (hotReloadingEnabled) {
            initShaderDirWatcher();
            startShaderDirWatcher();
        }
    }

    ShaderManager::~ShaderManager() {
        if (hotReloadingEnabled) {
            stopShaderDirWatcher();
        }
    }

    ShaderSource* ShaderManager::requestShaderSource(
        std::string shaderPath, ImplPipeline* requester
    ) {
        std::string formattedPath = formatPath(shaderPath);

        if (!sourceExists(formattedPath)) {
            addSource(device_, formattedPath, requester);
        }
        addDependency(shaderPath, requester);

        return getSource(formattedPath);
    }

    void ShaderManager::addSource(
        VkDevice device,
        const std::string& sourcePath,
        ImplPipeline* dependency
    ) {
        std::string formattedPath = formatPath(sourcePath);

        if (sourceExists(formattedPath)) {
            std::cout << "[Sumire::ShaderManager] WARNING: "
                << "Attempted to add already existing shader source to the shader map. ("
                << sourcePath << "). The existing entry was not modified." << std::endl;
        }
        else {
            std::unique_ptr<ShaderSource> newSource = std::make_unique<ShaderSource>(device, formattedPath);
            ShaderSource::SourceType newSourceType = newSource->getSourceType();

            if (hotReloadingEnabled) {
                resolveSourceParents(newSource.get(), dependency);
            }

            sources.emplace(formattedPath, std::move(newSource));
        }
    }

     void ShaderManager::addDependency(
        const std::string& sourcePath, 
        ImplPipeline* dependency
    ) {
        std::string formattedPath = formatPath(sourcePath);

        auto& dependencyEntry = dependencies.find(formattedPath);
        if (dependencyEntry == dependencies.end()) {
            dependencies.emplace(formattedPath, std::vector<ImplPipeline*>{ dependency });
        }
        else {
            dependencyEntry->second.push_back(dependency);
        }
    }

    void ShaderManager::hotReload(const std::string& sourcePath) {
        invalidateSourceAndChildren(sourcePath);
        revalidateSources();
        updatePipelines();
    }

    void ShaderManager::invalidateSourceAndChildren(const std::string& sourcePath) {
        std::string formattedPath = formatPath(sourcePath);

        auto& sourceEntry = sources.find(formattedPath);
        if (sourceEntry != sources.end()) {
            sourceEntry->second->invalidate();
        }
        else {
            std::cout << "[Sumire::ShaderManager] WARNING: Source <" << formattedPath
                << "> was modified but is not indexed - the source was not recompiled." << std::endl;
        }
    }

    void ShaderManager::revalidateSources() {
        std::set<ShaderSource*> revalidatedSources{};

        for (auto& kv : sources) {
            const std::string& pth = kv.first;
            ShaderSource* source   = kv.second.get();
            std::vector<ShaderSource*> updatedSources = source->revalidate(compiler.get());
            revalidatedSources.insert(updatedSources.begin(), updatedSources.end());
        }

        // Find dependencies which are now pending update
        for (auto& src : revalidatedSources) {
            std::vector<ImplPipeline*>& pipelines = dependencies.at(src->getSourcePath());
            pipelinesPendingUpdate.insert(pipelines.begin(), pipelines.end());
        }

    }

    void ShaderManager::updatePipelines() {
        // recreate pipelines
        for (auto& pipeline : pipelinesPendingUpdate) {
            if (VERBOSE_HOTRELOAD) {
                std::cout << "[Sumire::ShaderSource] Recreating pipeline 0x" << pipeline << std::endl;
            }
            pipeline->queuePipelineRecreation();
        }

        pipelinesPendingUpdate.clear();
    }

    void ShaderManager::resolveSourceParents(ShaderSource* source, ImplPipeline* dependency) {
        std::vector<std::string> includes = source->getSourceIncludes();
        for (std::string& inc : includes) {
            std::string parentPath = 
                util::relativeEngineFilepath(source->getSourcePath(), inc);

            ShaderSource* parent = requestShaderSource(parentPath, dependency);
            source->addParent(parent);
            parent->addChild(source);
        }
    }

    bool ShaderManager::sourceExists(const std::string& sourcePath) const {
        return sources.find(sourcePath) != sources.end();
    }

    ShaderSource* ShaderManager::getSource(const std::string& sourcePath) const {
        return sources.at(sourcePath).get();
    }

    std::string ShaderManager::formatPath(const std::string& path) const {
        std::filesystem::path fp = path;
        return fp.make_preferred().u8string();
    }
    
    // ---- Hot Reloading ----------------------------------------------------------------------------------------
    void ShaderManager::initShaderCompiler() {
        compiler = std::make_unique<ShaderCompiler>();
    }

    void ShaderManager::initShaderDirWatcher() {
        const std::string dir{ shaderDir };

        listener = std::make_unique<ShaderUpdateListener>(this);

#       ifdef _WIN32
        shaderDirWatcher = std::make_unique<watchers::FsWatcherWin>(
            dir,
            listener.get(),
            true
        );
#       endif
    }

    void ShaderManager::startShaderDirWatcher() {
#       ifdef _WIN32
        shaderDirWatcher->watch();
#       endif
    }

    void ShaderManager::stopShaderDirWatcher() {
#       ifdef _WIN32
        shaderDirWatcher->endWatch();
#       endif
    }

}