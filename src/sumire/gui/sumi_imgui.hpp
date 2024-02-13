#pragma once

#include <sumire/core/sumi_window.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_renderer.hpp>

namespace sumire {

    class SumiImgui {
        public:
            SumiImgui(SumiDevice &device, const SumiWindow& window, SumiRenderer &renderer);
            ~SumiImgui();

            SumiImgui(const SumiImgui&) = delete;
            SumiImgui& operator=(const SumiImgui&) = delete;

            void beginFrame();
            void drawStatWindow();
            void endFrame();
            void renderToCmdBuffer(VkCommandBuffer &buffer);

        private:
            void initImgui(SumiDevice &device, GLFWwindow *window, SumiRenderer &renderer);

            bool showStatsWindow{true};

            SumiDevice &sumiDevice;
            VkDescriptorPool imguiDescriptorPool;
    };

}
