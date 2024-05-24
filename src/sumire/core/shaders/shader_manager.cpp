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
                newSourceType == ShaderSource::SourceType::COMPUTE ||
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
            if (src->getSourceType() == ShaderSource::SourceType::GRAPHICS) {
                std::vector<SumiPipeline*>& pipelines = graphicsDependencies.at(src->getSourcePath());
                graphicsPipelinesPendingUpdate.insert(pipelines.begin(), pipelines.end());
            }
            else if (src->getSourceType() == ShaderSource::SourceType::COMPUTE) {
                std::vector<SumiComputePipeline*>& pipelines = computeDependencies.at(src->getSourcePath());
                computePipelinesPendingUpdate.insert(pipelines.begin(), pipelines.end());
            }
        }

    }

    void ShaderManager::updatePipelines() {
        // TODO: Only recreate pipelines for shaders which compilation was successful

        for (auto& pipeline : graphicsPipelinesPendingUpdate) {
            std::cout << "[Sumire::ShaderSource] Recreating graphics pipeline 0x" << pipeline << std::endl;
            // TODO: recreate pipelines with new ShaderSource VkShaderModule
        }

        for (auto& pipeline : computePipelinesPendingUpdate) {
            std::cout << "[Sumire::ShaderSource] Recreating compute pipeline 0x" << pipeline << std::endl;
        }

        graphicsPipelinesPendingUpdate.clear();
        computePipelinesPendingUpdate.clear();
    }

    void ShaderManager::resolveSourceParents(ShaderSource* source, SumiPipeline* dependency) {
        std::vector<std::string> includes = source->getSourceIncludes();
        for (std::string& inc : includes) {
            std::string parentPath = 
                util::relativeEngineFilepath(source->getSourcePath(), inc);

            ShaderSource* parent = requestShaderSource(parentPath, dependency);
            source->addParent(parent);
            parent->addChild(source);
        }
    }

    void ShaderManager::resolveSourceParents(ShaderSource* source, SumiComputePipeline* dependency) {
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