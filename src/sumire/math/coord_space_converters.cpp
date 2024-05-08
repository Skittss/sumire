#include <sumire/math/coord_space_converters.hpp>

namespace sumire {

    glm::vec4 clipToView(
        const glm::vec4& clip,
        const glm::mat4& invProjection
    ) {
        glm::vec4 view = invProjection * clip;
        return view / view.w;
    }

    glm::vec4 screenToView(
        const glm::vec4& screen,
        const glm::vec2& screenDim,
        const glm::mat4& invProjection
    ) {
        glm::vec2 ndc = glm::vec2{screen.x, screen.y} / screenDim;
        glm::vec4 clip = glm::vec4(
            glm::vec2(ndc.x, 1.0f - ndc.y) * 2.0f - 1.0f,
            screen.z, screen.w
        );

        return clipToView(clip, invProjection);
    }

}