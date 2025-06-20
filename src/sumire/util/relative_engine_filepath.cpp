#include <sumire/util/relative_engine_filepath.hpp>

namespace sumire::util {

    std::string relativeEngineFilepath(const std::string& base, const std::string& rel) {
        std::filesystem::path basePath{ base };
        std::filesystem::path relPath{ rel };
        std::filesystem::path combinedPath = std::filesystem::canonical(basePath.parent_path() / relPath);
        std::filesystem::path cleanedPath = combinedPath.make_preferred();
        std::filesystem::path enginePath = std::filesystem::relative(cleanedPath);

        return enginePath.string();
    }

}