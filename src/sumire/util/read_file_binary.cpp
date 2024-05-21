#include <sumire/util/read_file_binary.hpp>

namespace sumire::util {

    bool readFileBinary(
        const std::string& filepath, std::vector<char>& result
    ) {
        std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

        if (!file.is_open()) return false;

        size_t fileSize = static_cast<size_t>(file.tellg());
        result.resize(fileSize);

        file.seekg(0);
        file.read(result.data(), fileSize);

        file.close();
        return true;
    }

}