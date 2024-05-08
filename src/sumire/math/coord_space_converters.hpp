#pragma once

#include <glm/glm.hpp>

namespace sumire {

    glm::vec4 clipToView(
        const glm::vec4& clip, 
        const glm::mat4& invProjection
    );
    glm::vec4 screenToView(
        const glm::vec4& screen, 
        const glm::vec2& screenDim, 
        const glm::mat4& invProjection
    );

}