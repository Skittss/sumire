#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <sumire/core/rendering/sumi_object.hpp>

namespace sumire {

    enum SmCameraType {
        CAM_TYPE_PERSPECTIVE  = 0,
        CAM_TYPE_ORTHOGRAPHIC = 1
    };

    class SumiCamera {
    public:

        SumiCamera(float aspectRatio, SmCameraType cameraType);
        SumiCamera(float fovy, float aspect);
        SumiCamera(float l, float r, float top, float bot, float aspect, float zoom);
        ~SumiCamera();

        void setDefaultProjectionParams(float aspect, bool recomputeProjMatrix = false);
        void setDefaultOrthonormalBasis() { orthonormalBasis = glm::mat4{ 1.0f }; }

        void setOrthographicProjection(float l, float r, float top, float bot, float zoom);
        void setPerspectiveProjection(float fovy, float aspect);

        void setPos(glm::vec3 pos);
        void setPos(float x, float y, float z);
        void setPosX(float x);
        void setPosY(float y);
        void setPosZ(float z);
        glm::vec3 getPosition() const;

        void setRot(glm::vec3 rot);
        void setRot(float rotX, float rotY, float rotZ);
        void setRotX(float x);
        void setRotY(float y);
        void setRotZ(float z);
        glm::vec3 getRotation() const;

        // Orthonormal Bases Get & Set
        const glm::mat4& getOrthonormalBasis() const { return orthonormalBasis; }
        glm::vec3 getRight() const {
            return glm::vec3{ orthonormalBasis[0][0], orthonormalBasis[0][1], orthonormalBasis[0][2] };
        }
        glm::vec3 getUp() const {
            return glm::vec3{ orthonormalBasis[1][0], orthonormalBasis[1][1], orthonormalBasis[1][2] };
        }
        glm::vec3 getForward() const {
            return glm::vec3{ orthonormalBasis[2][0], orthonormalBasis[2][1], orthonormalBasis[2][2] };
        }
        void setRight(glm::vec3 right) {
            orthonormalBasis[0][0] = right.x; orthonormalBasis[0][1] = right.y; orthonormalBasis[0][2] = right.z;
        }
        void setUp(glm::vec3 up) {
            orthonormalBasis[1][0] = up.x; orthonormalBasis[1][1] = up.y; orthonormalBasis[1][2] = up.z;
        }
        void setForward(glm::vec3 forward) {
            orthonormalBasis[2][0] = forward.x; orthonormalBasis[2][1] = forward.y; orthonormalBasis[2][2] = forward.z;
        };

        // Near and Far get & set
        float near() const { return nearPlane; }
        float far() const { return farPlane; }
        void setNear(float dist, bool recomputeProjMatrix = false);
        void setFar(float dist, bool recomputeProjMatrix = false);

        // Projection get & set
        float getFovy() const { return persp_fovy; }
        void setFovy(float fovy, bool recomputeProjMatrix = false);
        float getAspect() const { return persp_aspect; }
        void setAspect(float aspect, bool recomputeProjMatrix = false);

        float getOrthoLeft() const { return ortho_l; }
        void setOrthoLeft(float orthoLeft, bool recomputeProjMatrix = false);
        float getOrthoRight() const { return ortho_r; }
        void setOrthoRight(float orthoRight, bool recomputeProjMatrix = false);
        float getOrthoTop() const { return ortho_top; }
        void setOrthoTop(float orthoTop, bool recomputeProjMatrix = false);
        float getOrthoBot() const { return ortho_bot; }
        void setOrthoBot(float orthoBot, bool recomputeProjMatrix = false);
        float getOrthoZoom() const { return ortho_zoom; }
        void setOrthoZoom(float orthoZoom, bool recomputeProjMatrix = false);

        void setViewDirection(glm::vec3 pos, glm::vec3 dir, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
        void setViewTarget(glm::vec3 pos, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
        void setViewYXZ(glm::vec3 pos, glm::vec3 rot);

        const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
        const glm::mat4& getViewMatrix() const { return viewMatrix; }

        SmCameraType getCameraType() const { return camType; }
        void setCameraType(SmCameraType projType, bool recomputeProjMatrix = false);

        void calculateProjectionMatrix();
        void FORCE_calculateProjectionMatrix();

        Transform3DComponent transform;
        bool projMatrixNeedsUpdate{ true };

    private:
        glm::mat4 projectionMatrix{ 1.0f };
        glm::mat4 viewMatrix{ 1.0f };
        glm::mat4 orthonormalBasis{ 1.0f };

        // Camera properties
        SmCameraType camType{ CAM_TYPE_PERSPECTIVE };

        // Shared
        float nearPlane{ 0.1f };
        float farPlane{ 1000.0f };

        // perspective proj. params
        float persp_fovy{ 1.0f };
        float persp_aspect{ 1.0f };

        // orthographic proj. params
        float ortho_l{ -1.0f };
        float ortho_r{ 1.0f };
        float ortho_top{ 1.0f };
        float ortho_bot{ -1.0f };
        float ortho_zoom{ 1.0f };
    };
    
}