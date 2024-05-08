#pragma once

#include <glm/glm.hpp>

namespace sumire {

    struct FrustumPlane {
        glm::vec3 normal{ 0.0 };
        float dist = 0.0;

        FrustumPlane() = default;
        FrustumPlane(glm::vec3 pos, glm::vec3 norm) {
            dist = glm::length(pos);
            normal = norm;
        }

        inline float sdf(const glm::vec3& p) const {
            return glm::dot(normal, p) - dist;
        }

        // Intersection test returning true if a sphere is fully behind the plane.
        inline bool intersectSphere(const glm::vec3& p, float r) const {
            return sdf(p) - r < 0;
        }
    };

    // Triangulate a frustum plane with view space points
    FrustumPlane computeFrustumPlane(
        const glm::vec3& p0,
        const glm::vec3& p1,
        const glm::vec3& p3
    );

}