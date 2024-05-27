#include <sumire/core/rendering/general/sumi_camera.hpp>

#include <cassert>
#include <limits.h>

//#include <glm/ext/matrix_clip_space.hpp>

namespace sumire {

    // Default camera for different types:
    //   Perspective:   50 fovy
    //   Orthographic: -1.0 top, 1.0 bot
    SumiCamera::SumiCamera(float aspect, SmCameraType cameraType) {
        camType = cameraType;
        setDefaultProjectionParams(aspect);
        FORCE_calculateProjectionMatrix();
    };

    // Creates a perspective camera
    SumiCamera::SumiCamera(float fovy, float aspect) {
        camType = CAM_TYPE_PERSPECTIVE;
        setDefaultProjectionParams(aspect);
        persp_fovy = fovy;
        persp_aspect = aspect;
        FORCE_calculateProjectionMatrix();
    };

    // Creates an orthographic camera
    SumiCamera::SumiCamera(float l, float r, float top, float bot, float aspect, float zoom) {
        camType = CAM_TYPE_ORTHOGRAPHIC;
        setDefaultProjectionParams(aspect);
        ortho_l = l;
        ortho_r = r;
        ortho_top = top;
        ortho_bot = bot;
        ortho_zoom = zoom;
        FORCE_calculateProjectionMatrix();
    };

    SumiCamera::~SumiCamera() {};

    void SumiCamera::setDefaultProjectionParams(float aspect, bool recomputeProjMatrix) {
        // Perspective
        persp_fovy = glm::radians(50.0f);
        persp_aspect = aspect;

        // Orthographic
        ortho_l = -aspect; 
        ortho_r =  aspect;
        ortho_top = 1.0f;
        ortho_bot = -1.0f;
        ortho_zoom = 1.0f;

        nearPlane = 0.1f;
        farPlane = 1000.0f;

        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setOrthographicProjection(float l, float r, float top, float bot, float zoom) {
        camType = CAM_TYPE_ORTHOGRAPHIC;

        projectionMatrix = glm::ortho(
            l / zoom, r / zoom, 
            bot / zoom, top / zoom,
            nearPlane, farPlane
        );
    }
    
    void SumiCamera::setPerspectiveProjection(float fovy, float aspect) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

        camType = CAM_TYPE_PERSPECTIVE;
        
        projectionMatrix = glm::perspective(
            fovy, aspect, 
            nearPlane, farPlane
        );
    }

    void SumiCamera::setNear(float dist, bool recomputeProjMatrix) {
        if (nearPlane == dist) return;
        
        nearPlane = dist;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setFar(float dist, bool recomputeProjMatrix) {
        if (farPlane == dist) return;

        farPlane = dist;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setFovy(float fovy, bool recomputeProjMatrix) {
        if (persp_fovy == fovy) return;

        persp_fovy = fovy;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setAspect(float aspect, bool recomputeProjMatrix) {
        if (persp_aspect == aspect) return;

        persp_aspect = aspect;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }
    
    void SumiCamera::setOrthoLeft(float orthoLeft, bool recomputeProjMatrix) {
        if (ortho_l == orthoLeft) return;

        ortho_l = orthoLeft;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setOrthoRight(float orthoRight, bool recomputeProjMatrix) {
        if (ortho_r == orthoRight) return;

        ortho_r = orthoRight;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setOrthoTop(float orthoTop, bool recomputeProjMatrix) {
        if (ortho_top == orthoTop) return;

        ortho_top = orthoTop;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setOrthoBot(float orthoBot, bool recomputeProjMatrix) {
        if (ortho_bot == orthoBot) return;

        ortho_bot = orthoBot;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    void SumiCamera::setOrthoZoom(float orthoZoom, bool recomputeProjMatrix) {
        if (ortho_zoom == orthoZoom) return;

        ortho_zoom = orthoZoom;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
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

        // TODO: It may be possible to elimite this matrix calculation with better linear algebra :P
        viewMatrix = orthonormalBasis * viewMatrix;
    }

    void SumiCamera::setCameraType(SmCameraType projType, bool recomputeProjMatrix) {
        if (camType == projType) return;

        camType = projType;
        projMatrixNeedsUpdate = true;
        if (recomputeProjMatrix) calculateProjectionMatrix();
    }

    // Calculates the camera's projection matrix if it needs updating.
    void SumiCamera::calculateProjectionMatrix() {
        // Only computes when a member variable has been changed. (checked by this flag)
        if (!projMatrixNeedsUpdate) return;

        if (camType == CAM_TYPE_PERSPECTIVE)
            setPerspectiveProjection(persp_fovy, persp_aspect);
        else 
            setOrthographicProjection(ortho_l, ortho_r, ortho_top, ortho_bot, ortho_zoom);

        projMatrixNeedsUpdate = false;
    }

    // Forces calculation of the camera's projection matrix even if it does not need an update (flag not set);
    void SumiCamera::FORCE_calculateProjectionMatrix() {
        if (camType == CAM_TYPE_PERSPECTIVE)
            setPerspectiveProjection(persp_fovy, persp_aspect);
        else 
            setOrthographicProjection(ortho_l, ortho_r, ortho_top, ortho_bot, ortho_zoom);

        projMatrixNeedsUpdate = false;
    }
}