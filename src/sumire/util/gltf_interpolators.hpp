#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sumire::util {

    enum GLTFinterpolationType { INTERP_LINEAR, INTERP_STEP, INTERP_CUBIC_SPLINE };

    glm::vec4 cubicSplineVec4(
        glm::vec4 v0, glm::vec4 v0_out_tangent, 
        glm::vec4 v1, glm::vec4 v1_in_tangent, 
        float t
    );
    glm::vec4 interpVec4(
        glm::vec4 v0, glm::vec4 v0_out_tangent,
        glm::vec4 v1, glm::vec4 v1_out_tangent,
        float t, 
        GLTFinterpolationType interpolationType
    );

    glm::quat cubicSplineQuat(
        glm::quat v0, glm::quat v0_out_tangent, 
        glm::quat v1, glm::quat v1_in_tangent, 
        float t
    );
    glm::quat interpQuat(
        glm::quat v0, glm::quat v0_out_tangent,
        glm::quat v1, glm::quat v1_in_tangent,
        float t, 
        GLTFinterpolationType interpolationType
    );
    
}