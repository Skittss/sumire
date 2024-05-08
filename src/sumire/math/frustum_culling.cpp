#include <sumire/math/frustum_culling.hpp>

namespace sumire {

    // https://www.3dgep.com/forward-plus/#Grid_Frustums#
    // Important Note: Assumes positive winding order of p0, p1, p2.
    FrustumPlane computeFrustumPlane(
        const glm::vec3& p0,
        const glm::vec3& p1,
        const glm::vec3& p2
    ) {
        glm::vec3 v0 = p1 - p0;
        glm::vec3 v2 = p2 - p0;

        FrustumPlane plane{};
        plane.normal = glm::normalize(glm::cross(v0, v2));
        plane.dist = glm::dot(plane.normal, p0);

        return plane;
    }

}