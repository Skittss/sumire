#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_attachment.hpp>

#include <memory>

namespace sumire {

    class SumiGbuffer {
        public:

            SumiGbuffer(
                SumiDevice &device, 
                VkExtent2D extent,
                VkImageUsageFlags extraFlags = 0x0
            );
            ~SumiGbuffer();

            SumiGbuffer(const SumiGbuffer&) = delete;
		    SumiGbuffer& operator=(const SumiGbuffer&) = delete;

            SumiAttachment* positionAttachment() const { return position.get(); }
            SumiAttachment* normalAttachment() const { return normal.get(); }
            SumiAttachment* albedoAttachment() const { return albedo.get(); }

        private:
            void createAttachments(VkImageUsageFlags extraFlags);

            SumiDevice &sumiDevice;
            VkExtent2D extent;

            std::unique_ptr<SumiAttachment> position;
            std::unique_ptr<SumiAttachment> normal;
            std::unique_ptr<SumiAttachment> albedo;
    };

}