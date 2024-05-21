#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace sumire::util {

    bool readFileBinary(
        const std::string& filepath, std::vector<char>& result
    );

}