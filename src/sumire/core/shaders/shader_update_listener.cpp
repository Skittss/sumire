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
        // Code generating actions
        case watchers::FsWatchAction::FS_RENAME_NEW:
        case watchers::FsWatchAction::FS_MODIFIED: {
            std::filesystem::path filePath{ filename };
            std::filesystem::path ext{ filePath.extension()};
            bool isShaderSource = validShaderSourceExt(ext.string());

            if (isShaderSource) {
                std::filesystem::path dirPath{ dir };
                std::filesystem::path combinedPath = dirPath / filePath ;

                shaderManager->hotReload(combinedPath.string());
            }

        } break;
        // Others for debug logging
        //case watchers::FsWatchAction::FS_RENAME_OLD: {
        //    std::cout << "[Sumire::ShaderManager] INFO: File renamed from: <" + filename + ">." << std::endl;
        //} break;
        //case watchers::FsWatchAction::FS_ADD: {
        //    std::cout << "[Sumire::ShaderManager] INFO: File added: <" + filename + ">." << std::endl;
        //} break;
        //case watchers::FsWatchAction::FS_DELETE: {
        //    std::cout << "[Sumire::ShaderManager] INFO: File deleted: <" + filename + ">." << std::endl;
        //} break;
        default:
        break;
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