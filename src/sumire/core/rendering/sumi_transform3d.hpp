#pragma once

#include <glm/gtc/matrix_transform.hpp>

class Transform3DComponent {

    public:
        glm::vec3 getTranslation() const { return translation; }
        glm::vec3 getScale() const { return scale; }
        glm::vec3 getRotation() const { return rotation; }

        void setTranslation(glm::vec3 translation) { 
            if (this->translation == translation) return;

            this->translation = translation;
            needsCacheUpdate = true;
        }
        void setScale(glm::vec3 scale) {
            if (this->scale == scale) return;

            this->scale = scale;
            needsCacheUpdate = true;
        }
        void setRotation(glm::vec3 rotation) {
            if (this->rotation == rotation) return;

            this->rotation = rotation;
            needsCacheUpdate = true;
        }

        glm::mat4 normalMatrix() {
            if (needsCacheUpdate) calculateCacheMatrices();
            return cachedNormalMatrix;
        }

        glm::mat4 modelMatrix() {
            if (needsCacheUpdate) calculateCacheMatrices();
            return cachedModelMatrix;
        }

    private:
        glm::vec3 translation{0.0f};
        glm::vec3 scale{1.0f};
        glm::vec3 rotation{0.0f};

        glm::mat4 cachedModelMatrix{1.0f};
        glm::mat4 cachedNormalMatrix{1.0f};
        bool needsCacheUpdate = true;

        void calculateCacheMatrices() {

            // This function manually calculates YXZ Tait-bryer model and normal matrices
            //  for performance.
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);

            // Model Matrix
            cachedModelMatrix = glm::mat4{
            {
                scale.x * (c1 * c3 + s1 * s2 * s3),
                scale.x * (c2 * s3),
                scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },
            {
                scale.y * (c3 * s1 * s2 - c1 * s3),
                scale.y * (c2 * c3),
                scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },
            {
                scale.z * (c2 * s1),
                scale.z * (-s2),
                scale.z * (c1 * c2),
                0.0f,
            },
            {translation.x, translation.y, translation.z, 1.0f}};


            // Normal Matrix
            const glm::vec3 invScale = 1.0f / scale;
            cachedNormalMatrix = glm::mat4{
            {
                invScale.x * (c1 * c3 + s1 * s2 * s3),
                invScale.x * (c2 * s3),
                invScale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f
            },
            {
                invScale.y * (c3 * s1 * s2 - c1 * s3),
                invScale.y * (c2 * c3),
                invScale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f
            },
            {
                invScale.z * (c2 * s1),
                invScale.z * (-s2),
                invScale.z * (c1 * c2),
                0.0f
            },
            {0.0f, 0.0f, 0.0f, 1.0f}};

            needsCacheUpdate = false;
        }


};