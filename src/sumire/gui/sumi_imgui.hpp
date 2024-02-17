#pragma once

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <sumire/core/sumi_window.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_renderer.hpp>
#include <sumire/core/sumi_frame_info.hpp>
#include <sumire/core/sumi_object.hpp>

namespace sumire {

    class SumiImgui {
        public:
            SumiImgui(SumiRenderer &renderer);
            ~SumiImgui();

            SumiImgui(const SumiImgui&) = delete;
            SumiImgui& operator=(const SumiImgui&) = delete;

            void beginFrame();
            void endFrame();
            void renderToCmdBuffer(VkCommandBuffer &buffer);

            void drawStatWindow(FrameInfo &frameInfo);

            ImGuiIO& getIO();

        private:
            void initImgui();
            void drawTransformUI(Transform3DComponent &transform);

            SumiRenderer &sumiRenderer;
            VkDescriptorPool imguiDescriptorPool;

            int resetProjParamsCounter{0};
    };

}