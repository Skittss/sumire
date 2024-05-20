#include <sumire/core/shaders/shader_manager.hpp>

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
        std::string shaderPath, SumiPipeline* requester
    ) {
        std::string formattedPath = formatPath(shaderPath);

        if (!sourceExists(formattedPath)) {
            addGraphicsSource(device_, formattedPath, requester);
        }
        addGraphicsDependency(shaderPath, requester);

        return getSource(formattedPath);
    }

    ShaderSource* ShaderManager::requestShaderSource(
        std::string shaderPath, SumiComputePipeline* requester
    ) {
        std::string formattedPath = formatPath(shaderPath);

        if (!sourceExists(formattedPath)) {
            addComputeSource(device_, formattedPath, requester);
        }
        addComputeDependency(shaderPath, requester);

        return getSource(formattedPath);
    }

    void ShaderManager::addGraphicsSource(
        VkDevice device,
        const std::string& sourcePath,
        SumiPipeline* dependency
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
            assert(
                newSourceType == ShaderSource::SourceType::GRAPHICS ||
                newSourceType == ShaderSource::SourceType::INCLUDE
                && "Loaded incompatible shader source type when attempting to add a graphics source."
            );

            if (hotReloadingEnabled) {
                resolveSourceParents(newSource.get(), dependency);
            }

            sources.emplace(formattedPath, std::move(newSource));
        }
    }
    
    void ShaderManager::addGraphicsDependency(
        const std::string& sourcePath, 
        SumiPipeline* dependency
    ) {
        std::string formattedPath = formatPath(sourcePath);

        auto& dependencyEntry = graphicsDependencies.find(formattedPath);
        if (dependencyEntry == graphicsDependencies.end()) {
            graphicsDependencies.emplace(formattedPath, std::vector<SumiPipeline*>{ dependency });
        }
        else {
            dependencyEntry->second.push_back(dependency);
        }
    }

    void ShaderManager::addComputeSource(
        VkDevice device,
        const std::string& sourcePath,
        SumiComputePipeline* dependency
    ) {
        std::string formattedPath = formatPath(sourcePath);

        if (sourceExists(formattedPath)) {
            std::cout << "[Sumire::ShaderManager] WARNING: "
                << "Attempted to add already existing shader source to the shader map. ("
                << formattedPath << "). The existing entry was not modified." << std::endl;
        }
        else {
            std::unique_ptr<ShaderSource> newSource = std::make_unique<ShaderSource>(device, formattedPath);
            ShaderSource::SourceType newSourceType = newSource->getSourceType();
            assert(
                newSource->getSourceType() == ShaderSource::SourceType::COMPUTE ||
                newSourceType == ShaderSource::SourceType::INCLUDE
                && "Loaded incompatible shader source type when attempting to add a compute source.");

            if (hotReloadingEnabled) {
                resolveSourceParents(newSource.get(), dependency);
            }

            sources.emplace(formattedPath, std::move(newSource));
        }
    }

    void ShaderManager::addComputeDependency(
        const std::string& sourcePath,
        SumiComputePipeline* dependency
    ) {
        std::string formattedPath = formatPath(sourcePath);

        auto& dependencyEntry = computeDependencies.find(formattedPath);
        if (dependencyEntry == computeDependencies.end()) {
            computeDependencies.emplace(formattedPath, std::vector<SumiComputePipeline*>{ dependency });
        }
        else {
            dependencyEntry->second.push_back(dependency);
        }
    }

    void ShaderManager::resolveSourceParents(ShaderSource* source, SumiPipeline* dependency) {
        std::vector<std::string> includes = source->getSourceIncludes();
        for (std::string& inc : includes) {
            std::filesystem::path sourcePath{ source->getSourcePath() };
            std::filesystem::path incPath{ inc };
            std::filesystem::path combinedPath = std::filesystem::canonical(sourcePath.parent_path() / incPath);
            std::filesystem::path cleanedPath = combinedPath.make_preferred();
            std::filesystem::path enginePath = std::filesystem::relative(cleanedPath);
            ShaderSource* parent = requestShaderSource(enginePath.u8string(), dependency);
            source->addParent(parent);
        }
    }

    void ShaderManager::resolveSourceParents(ShaderSource* source, SumiComputePipeline* dependency) {
        std::vector<std::string> includes = source->getSourceIncludes();
        for (std::string& inc : includes) {
            std::filesystem::path sourcePath{ source->getSourcePath() };
            std::filesystem::path incPath{ inc };
            std::filesystem::path combinedPath = std::filesystem::canonical(sourcePath.parent_path() / incPath);
            std::filesystem::path cleanedPath = combinedPath.make_preferred();
            std::filesystem::path enginePath = std::filesystem::relative(cleanedPath);
            ShaderSource* parent = requestShaderSource(enginePath.u8string(), dependency);
            source->addParent(parent);
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
    void ShaderManager::ShaderUpdateListener::handleFileAction(
        watchers::FsWatchAction action,
        const std::string& dir,
        const std::string& filename
    ) {
        switch (action) {
        case watchers::FsWatchAction::FS_MODIFIED: {
            std::cout << "[Sumire::ShaderManager] INFO: File <" + filename + "> modified." << std::endl;
        } break;
        case watchers::FsWatchAction::FS_MOVED: {
            std::cout << "[Sumire::ShaderManager] INFO: File <" + filename + "> moved." << std::endl;
        } break;
        case watchers::FsWatchAction::FS_ADD: {
            std::cout << "[Sumire::ShaderManager] INFO: File <" + filename + "> added." << std::endl;
        } break;
        case watchers::FsWatchAction::FS_DELETE: {
            std::cout << "[Sumire::ShaderManager] INFO: File <" + filename + "> deleted." << std::endl;
        } break;
        default:
            throw std::runtime_error("[Sumire::ShaderManager] Received invalid FsWatchAction.");
        }
    }

    void ShaderManager::initShaderDirWatcher() {
        const std::string dir{ shaderDir };

        listener = std::make_unique<ShaderUpdateListener>();

#ifdef _WIN32
        shaderDirWatcher = std::make_unique<watchers::FsWatcherWin>(
            dir,
            listener.get(),
            true
        );
#endif
    }

    void ShaderManager::startShaderDirWatcher() {
#ifdef _WIN32
        shaderDirWatcher->watch();
#endif
    }

    void ShaderManager::stopShaderDirWatcher() {
#ifdef _WIN32
        shaderDirWatcher->endWatch();
#endif
    }

}