#include <sumire/util/gltf_interpolators.hpp>

namespace sumire::util {

    glm::vec4 interpVec4(glm::vec4 a, glm::vec4 b, GLTFinterpolationType interpolationType) {
        glm::vec4 result;
        switch(interpolationType) {
            case GLTFinterpolationType::INTERP_LINEAR: {

            }
            break;
            case GLTFinterpolationType::INTERP_STEP: {

            }
            break;
            case GLTFinterpolationType::INTERP_CUBIC_SPLINE: {

            }
            break;
        }
        return result;
    }

    glm::quat interpQuat(glm::quat a, glm::quat b, GLTFinterpolationType interpolationType) {
        glm::quat result;
        switch(interpolationType) {
            case GLTFinterpolationType::INTERP_LINEAR: {

            }
            break;
            case GLTFinterpolationType::INTERP_STEP: {

            }
            break;
            case GLTFinterpolationType::INTERP_CUBIC_SPLINE: {

            }
            break;
        }
        return result;
    }

}