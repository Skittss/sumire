#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

namespace sumire {

    class SumiGbuffer {
        public:

            SumiGbuffer(SumiDevice &device, uint32_t width, uint32_t height);
            ~SumiGbuffer();

            SumiGbuffer(const SumiGbuffer&) = delete;
		    SumiGbuffer& operator=(const SumiGbuffer&) = delete;

            void beginRenderPass(VkCommandBuffer commandBuffer);
            void endRenderPass(VkCommandBuffer commandBuffer);
            VkRenderPass getRenderPass() const { return renderPass; }

            void submitCommandBuffer(
                const VkCommandBuffer* buffer,
                const uint32_t waitSemaphoreCount,
                const VkSemaphore *waitSemaphores,
                const VkPipelineStageFlags *waitDstStageMask
            );

            VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }

            struct GbufferAttachment {
                VkImage image;
                VkImageView view;
                VkDeviceMemory memory;
                VkFormat format;
            };

            // Color attachments for position/albedo/normal, & depth
            GbufferAttachment position;
            GbufferAttachment albedo;
            GbufferAttachment normal;
            GbufferAttachment depth;

        private:


            void init();
            void createAttachments();
            void createAttachment(
                VkFormat format, VkImageUsageFlagBits usage, GbufferAttachment &attachment);
            void destroyAttachment(GbufferAttachment &attachment);
            void createRenderPass();
            void createFramebuffer();
            void createSyncObjects();

            SumiDevice &sumiDevice;
            uint32_t width;
            uint32_t height;

            // VK handles
            VkRenderPass renderPass = VK_NULL_HANDLE;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
            
            // Render pass info
            bool renderPassStarted = false;
    };

}