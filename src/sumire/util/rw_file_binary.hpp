#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace sumire::util {

    bool readFileBinary(
        const std::string& filepath, std::vector<char>& result);

    bool writeFileBinary(
        const std::string& filepath, const std::vector<char>& data);

    bool writeFileBinary(
        const std::string& filepath, const char* data, size_t dataSize);

}