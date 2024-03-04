#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace sumire::util {

    enum GLTFinterpolationType { INTERP_LINEAR, INTERP_STEP, INTERP_CUBIC_SPLINE };

    glm::vec4 interpVec4(glm::vec4 a, glm::vec4 b, GLTFinterpolationType interpolationType);
    glm::quat interpQuat(glm::quat a, glm::quat b, GLTFinterpolationType interpolationType);

}