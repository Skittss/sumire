#include <sumire/core/shaders/shader_update_listener.hpp>

#include <iostream>

namespace sumire {

    ShaderUpdateListener::ShaderUpdateListener(
        ShaderManager* shaderManager
    ) : shaderManager{ shaderManager } {}

    void ShaderUpdateListener::handleFileAction(
        watchers::FsWatchAction action,
        const std::string& dir,
        const std::string& filename
    ) {
        switch (action) {
        case watchers::FsWatchAction::FS_MODIFIED: {
            std::filesystem::path filePath{ filename };
            std::filesystem::path ext{ filePath.extension()};
            bool isShaderSource = validShaderSourceExt(ext.u8string());

            if (isShaderSource) {
                std::filesystem::path dirPath{ dir };
                std::filesystem::path combinedPath = dirPath / filePath ;

                shaderManager->hotReload(combinedPath.u8string());
            }

        } break;
        case watchers::FsWatchAction::FS_MOVED: {
            std::cout << "[Sumire::ShaderManager] INFO: File move in <" + filename + ">." << std::endl;
        } break;
        case watchers::FsWatchAction::FS_ADD: {
            std::cout << "[Sumire::ShaderManager] INFO: File added in <" + filename + ">." << std::endl;
        } break;
        case watchers::FsWatchAction::FS_DELETE: {
            std::cout << "[Sumire::ShaderManager] INFO: File deleted in <" + filename + ">." << std::endl;
        } break;
        default:
            throw std::runtime_error("[Sumire::ShaderManager] Received invalid FsWatchAction.");
        }
    }

    bool ShaderUpdateListener::validShaderSourceExt(const std::string& ext) {
        return (
            ext == ".glsl" ||
            ext == ".vert" ||
            ext == ".frag" ||
            ext == ".comp"
        );
    }

}