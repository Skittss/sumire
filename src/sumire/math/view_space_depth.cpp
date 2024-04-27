#include <sumire/math/view_space_depth.hpp>

namespace sumire {

    // Calculates view space depth orthogonal to the camera's near plane
    float calculateOrthogonalViewSpaceDepth(
        glm::vec3 pos, glm::mat4 view, glm::vec3* outViewSpacePos
    ) {
        glm::vec4 viewSpacePos = view * glm::vec4(pos, 1.0f);
        viewSpacePos /= viewSpacePos.w;

        if (outViewSpacePos != nullptr) *outViewSpacePos = glm::vec3(viewSpacePos);

        // Camera is -z oriented so negate z-value
        return -viewSpacePos.z;
    }

    // Calculates view space depth along the viewing ray of a point (affected by perspective projection),
    //  and truncates via the near plane.
    float calculatePerspectiveViewSpaceDepth(
        glm::vec3 pos, glm::mat4 view, float near, glm::vec3* outViewSpacePos
    ) {
        // Perspective depth calculation in view space
        //  This is the length of the view position (A) -> point (B) vector truncated by the near plane 
        //  intersection:
        //    t = ( near - n . (A) ) / ( n . (B - A) )
        //  We are in view space, so A is at the origin and is zero-valued. This reduces the
        //  calculation to:
        //    t = near / (n . B)

        glm::vec4 viewSpacePos = view * glm::vec4(pos, 1.0f);
        viewSpacePos /= viewSpacePos.w;

        glm::vec3 B = glm::vec3(viewSpacePos); // .xyz
        if (outViewSpacePos != nullptr) *outViewSpacePos = B;

        glm::vec3 viewDir = B;
        constexpr glm::vec3 normal = { 0.0f, 0.0f, -1.0f }; // Z-oriented (camera matrix is -z)
        float viewDotNormal = glm::dot(normal, viewDir);

        float t = near / glm::dot(normal, viewDir);

        glm::vec3 intersection = t * viewDir; // A = 0
        glm::vec3 zVector = B - intersection;

        return glm::sign(t) * glm::length(zVector);

    }

}