#include <sumire/util/gltf_interpolators.hpp>

namespace sumire::util {

    // See the glTF 2.0 spec for more details on these interpolation methods.
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#appendix-c-interpolation

    glm::vec4 cubicSplineVec4(
        glm::vec4 v0, glm::vec4 v0_out_tangent, 
        glm::vec4 v1, glm::vec4 v1_in_tangent, 
        float t
    ) {
        float t2 = t*t;
        float t3 = t2*t;

        return (2.0f*t3 - 3.0f*t2 + 1.0f) * v0 
             + (t3 - 2.0f*t2 + t) * v0_out_tangent 
             + (-2.0f*t3 + 3.0f*t2) * v1 
             + (t3 - t2) * v1_in_tangent;
    }

    glm::vec4 interpVec4(
        glm::vec4 v0, glm::vec4 v0_out_tangent,
        glm::vec4 v1, glm::vec4 v1_out_tangent,
        float t, 
        GLTFinterpolationType interpolationType
    ) {
        glm::vec4 result;
        switch(interpolationType) {
            // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-lerp
            case GLTFinterpolationType::INTERP_LINEAR: {
                result = glm::mix(v0, v1, t);
            }
            break;
            // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_step_interpolation
            case GLTFinterpolationType::INTERP_STEP: {
                result = v0;
            }
            break;
            // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic
            case GLTFinterpolationType::INTERP_CUBIC_SPLINE: {
                result = cubicSplineVec4(v0, v0_out_tangent, v1, v1_out_tangent, t);
            }
            break;
        }
        return result;
    }

    // Note: glTF spec states that the resulting quaternion MUST be normalized before applying this qauternion
    //       to a node's rotation.
    glm::quat cubicSplineQuat(        
        glm::quat v0, glm::quat v0_out_tangent, 
        glm::quat v1, glm::quat v1_in_tangent, 
        float t
    ) {
        float t2 = t*t;
        float t3 = t2*t;

        return (2.0f*t3 - 3.0f*t2 + 1.0f) * v0 
             + (t3 - 2.0f*t2 + t) * v0_out_tangent 
             + (-2.0f*t3 + 3.0f*t2) * v1 
             + (t3 - t2) * v1_in_tangent;
    }

    glm::quat interpQuat(
        glm::quat v0, glm::quat v0_out_tangent,
        glm::quat v1, glm::quat v1_out_tangent,
        float t, 
        GLTFinterpolationType interpolationType
    ) {
        glm::quat result;
        switch(interpolationType) {
            // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-slerp
            case GLTFinterpolationType::INTERP_LINEAR: {
                result = glm::normalize(glm::slerp(v0, v1, t));
            }
            break;
            case GLTFinterpolationType::INTERP_STEP: {
                result = v0;
            }
            break;
            case GLTFinterpolationType::INTERP_CUBIC_SPLINE: {
                result = glm::normalize(cubicSplineQuat(v0, v0_out_tangent, v1, v1_out_tangent, t));
            }
            break;
        }
        return result;
    }

}