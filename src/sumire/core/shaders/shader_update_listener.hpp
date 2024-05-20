#pragma once 

#include <sumire/watchers/fs_watch_listener.hpp>
#include <sumire/core/shaders/shader_manager.hpp>

#include <string>

namespace sumire {

    class ShaderManager;

    class ShaderUpdateListener : public watchers::FsWatchListener {
    public:
        ShaderUpdateListener(ShaderManager* shaderManager);

        void handleFileAction(
            watchers::FsWatchAction action,
            const std::string& dir,
            const std::string& filename
        ) override;

    private:

        bool validShaderSourceExt(const std::string& ext);

        ShaderManager* shaderManager;
    };

}