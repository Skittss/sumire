#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <sumire/core/sumi_object.hpp>

namespace sumire {

    enum SmCameraType {
        CAM_TYPE_PERSPECTIVE  = 0,
        CAM_TYPE_ORTHOGRAPHIC = 1
    };

    class SumiCamera {
    public:

        SumiCamera(float aspectRatio, SmCameraType cameraType);
        SumiCamera(float fovy, float aspect);
        SumiCamera(float l, float r, float top, float bot, float aspect);
        ~SumiCamera();

        void setDefaultProjectionParams(float aspect, bool recomputeProjMatrix = false);

        void setOrthographicProjection(float l, float r, float top, float bot);
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
        
        glm::vec3 getUpVector() const;

        void setNear(float dist, bool recomputeProjMatrix = false);
        void setFar(float dist, bool recomputeProjMatrix = false);

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

        void setViewDirection(glm::vec3 pos, glm::vec3 dir, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
        void setViewTarget(glm::vec3 pos, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
        void setViewYXZ(glm::vec3 pos, glm::vec3 rot);

        const glm::mat4& getProjectionMatrix() const { return projectionMatrix; }
        const glm::mat4& getViewMatrix() const { return viewMatrix; }

        void setCameraType(SmCameraType projType, bool recomputeProjMatrix = false);
        void calculateProjectionMatrix();
        void FORCE_calculateProjectionMatrix();

        Transform3DComponent transform;
        bool projMatrixNeedsUpdate{true};

    private:
        glm::mat4 projectionMatrix{1.0f};
        glm::mat4 viewMatrix{1.0f};

        // Camera properties
        SmCameraType camType{CAM_TYPE_PERSPECTIVE};

        // Shared
        float nearPlane{0.1f};
        float farPlane{1000.0f};

        // perspective proj. params
        float persp_fovy;
        float persp_aspect;

        // orthographic proj. params
        float ortho_l;
        float ortho_r;
        float ortho_top;
        float ortho_bot;

    };
    
}