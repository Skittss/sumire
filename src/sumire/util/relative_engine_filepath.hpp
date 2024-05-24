#pragma once

#include <string>
#include <filesystem>

namespace sumire::util {

    std::string relativeEngineFilepath(const std::string& baseFile, const std::string& relativeFile);

}