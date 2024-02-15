#include <sumire/core/sumi_camera.hpp>

#include <cassert>
#include <limits.h>

//#include <glm/ext/matrix_clip_space.hpp>

namespace sumire {

    SumiCamera::SumiCamera() {};
    SumiCamera::~SumiCamera() {};

    void SumiCamera::setOrthographicProjection(float l, float r, float top, float bot) {
        camType = CAM_TYPE_ORTHOGRAPHIC;
        //projectionMatrix = glm::ortho(ortho_l, ortho_r, top, bot, nearPlane, farPlane);
        //return;

        projectionMatrix = glm::mat4{1.0f};
        projectionMatrix[0][0] = 2.0f / (r - l);
        projectionMatrix[1][1] = 2.0f / (bot - top);
        projectionMatrix[2][2] = 1.0f / (farPlane - nearPlane);
        projectionMatrix[3][0] = -(r + l) / (r - l);
        projectionMatrix[3][1] = -(bot + top) / (bot - top);
        projectionMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
    }
    
    void SumiCamera::setPerspectiveProjection(float fovy, float aspect) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

        camType = CAM_TYPE_PERSPECTIVE;
        //projectionMatrix = glm::perspective(fovy, aspect, nearPlane, farPlane);
        //return;
        
        const float tanHalfFovy = tan(fovy / 2.0f);
        projectionMatrix = glm::mat4{0.0f};
        projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovy);
        projectionMatrix[1][1] = 1.0f / (tanHalfFovy);
        projectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
        projectionMatrix[2][3] = 1.0f;
        projectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
    }

    glm::vec3 SumiCamera::getPosition() const {
        return {viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]};
    }

    void SumiCamera::setNear(float dist) {
        nearPlane = dist;
        calculateProjectionMatrix();
    }

    void SumiCamera::setFar(float dist) {
        farPlane = dist;
        calculateProjectionMatrix();
    }

    void SumiCamera::setViewDirection(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) {
        // Orthonormal basis
        const glm::vec3 w{glm::normalize(dir)};
        const glm::vec3 u{glm::normalize(glm::cross(w, up))};
        const glm::vec3 v{glm::cross(w, u)};

        viewMatrix = glm::mat4{1.0};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, pos);
        viewMatrix[3][1] = -glm::dot(v, pos);
        viewMatrix[3][2] = -glm::dot(w, pos);
    }

    void SumiCamera::setViewTarget(glm::vec3 pos, glm::vec3 target, glm::vec3 up) {
        setViewDirection(pos, target - pos, up);
    }

    void SumiCamera::setViewYXZ(glm::vec3 pos, glm::vec3 rot) {
        const float c3 = glm::cos(rot.z);
        const float s3 = glm::sin(rot.z);
        const float c2 = glm::cos(rot.x);
        const float s2 = glm::sin(rot.x);
        const float c1 = glm::cos(rot.y);
        const float s1 = glm::sin(rot.y);
        const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
        const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
        const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
        viewMatrix = glm::mat4{1.f};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, pos);
        viewMatrix[3][1] = -glm::dot(v, pos);
        viewMatrix[3][2] = -glm::dot(w, pos);
    }

    void SumiCamera::calculateProjectionMatrix() {
        if (camType == CAM_TYPE_PERSPECTIVE)
            setPerspectiveProjection(fovy, aspect);
        else 
            setOrthographicProjection(ortho_l, ortho_r, top, bot);
    }
}