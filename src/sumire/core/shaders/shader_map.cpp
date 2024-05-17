#include <sumire/core/shaders/shader_map.hpp>

#include <iostream>
#include <cassert>

namespace sumire {

    void ShaderMap::addSource(VkDevice device, const std::string& sourcePath) {
        assert(sumiDevice != nullptr && "Cannot add a source with no SumiDevice specified.");

        if (sourceExists(sourcePath)) {
            std::cout << "[Sumire::ShaderMap] WARNING: "
                << "Attempted to add already existing shader source to the shader map. ("
                << sourcePath << "). The existing entry was not modified." << std::endl;
        }
        else {
            ShaderSource newSource = ShaderSource{ device, sourcePath };
            sources.emplace(sourcePath, std::move(newSource));
        }
    }

    bool ShaderMap::sourceExists(const std::string& sourcePath) {
        return sources.find(sourcePath) != sources.end();
    }

    ShaderSource* ShaderMap::getSource(const std::string& sourcePath) {
        return &sources.at(sourcePath);
    }

}