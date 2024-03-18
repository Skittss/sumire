#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

namespace sumire {

    class SumiGbuffer {
        public:

            SumiGbuffer(SumiDevice &device, uint32_t width, uint32_t height);
            ~SumiGbuffer();

            SumiGbuffer(const SumiGbuffer&) = delete;
		    SumiGbuffer& operator=(const SumiGbuffer&) = delete;

        private:
            struct GbufferAttachment {
                VkImage image;
                VkImageView view;
                VkDeviceMemory memory;
                VkFormat format;
            };

            void init();

            void createAttachments();
            void createAttachment(
                VkFormat format, VkImageUsageFlagBits usage, GbufferAttachment &attachment);
            void destroyAttachment(GbufferAttachment &attachment);

            void createRenderPass();
        
            SumiDevice &sumiDevice;
            uint32_t width;
            uint32_t height;

            // Color attachments for position/albedo/normal, & depth
            GbufferAttachment position;
            GbufferAttachment albedo;
            GbufferAttachment normal;
            GbufferAttachment depth;

            // Render pass
            VkRenderPass renderPass;
    };

}