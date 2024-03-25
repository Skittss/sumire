#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/models/primitive.hpp>

namespace sumire {

    struct Mesh {
        std::vector<std::unique_ptr<Primitive>> primitives;

        struct UniformData {
            glm::mat4 matrix;
            glm::mat4 normalMatrix;
            int nJoints{ 0 };
        } uniforms;

        struct JointData {
            glm::mat4 jointMatrix;
            glm::mat4 jointNormalMatrix;
        };
        
        // Unform Buffer & Descriptor Set
        // Only one buffer as constant between swap-chain images
        std::unique_ptr<SumiBuffer> uniformBuffer = VK_NULL_HANDLE; 
        // for skinning & animation, stored in host-coherent memory not local gpu memory.
        std::unique_ptr<SumiBuffer> jointBuffer = VK_NULL_HANDLE; 
        // Note: This descriptor is PARTIALLY_BOUND as jointBuffer can be VK_NULL_HANDLE if there is no skin.
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        Mesh(SumiDevice &device, glm::mat4 matrix);
        ~Mesh() {
            primitives.clear();
            uniformBuffer = nullptr;
        }

        void initJointBuffer(SumiDevice &device, uint32_t nJoints);
    };

}