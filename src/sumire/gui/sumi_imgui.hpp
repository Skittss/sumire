#pragma once

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/rendering/sumi_renderer.hpp>
#include <sumire/core/rendering/sumi_frame_info.hpp>
#include <sumire/core/rendering/sumi_object.hpp>
#include <sumire/core/render_systems/grid_rendersys.hpp>

#include <sumire/input/sumi_kbm_controller.hpp>

namespace sumire {

    struct gridAdjustableParams {

    };

    class SumiImgui {
        public:
            SumiImgui(
                SumiRenderer &renderer,
                VkRenderPass renderPass,
                uint32_t subpassIdx,
                VkQueue workQueue
            );
            ~SumiImgui();

            SumiImgui(const SumiImgui&) = delete;
            SumiImgui& operator=(const SumiImgui&) = delete;

            void beginFrame();
            void endFrame();
            void renderToCmdBuffer(VkCommandBuffer &buffer);

            void drawStatWindow(FrameInfo &frameInfo, SumiKBMcontroller &cameraController);

            ImGuiIO& getIO();
            void ignoreMouse() { getIO().ConfigFlags |= ImGuiConfigFlags_NoMouse; }
            void enableMouse() { getIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse; }

            // Grid Params
            GridRendersys::GridUBOdata getGridUboData();
            float gridOpacity{1.0f};
            float gridTileSize{10.0f};
            float gridFogNear{0.01};
            float gridFogFar{1.0};
            ImVec4 gridMinorLineCol{0.2f, 0.2f, 0.2f, 1.0f};
            ImVec4 gridXaxisCol{1.0f, 0.2f, 0.2f, 1.0f};
            ImVec4 gridZaxisCol{0.2f, 0.2f, 1.0f, 1.0f};
            bool showGrid{true};

        private:
            void initImgui(VkRenderPass renderPass, uint32_t subpassIdx, VkQueue workQueue);
            void drawConfigUI(SumiKBMcontroller &cameraController);
            void drawSceneUI(FrameInfo &frameInfo);
            void drawTransformUI(Transform3DComponent &transform, bool includeScale = true);

            SumiRenderer &sumiRenderer;
            VkDescriptorPool imguiDescriptorPool;

            int resetProjParamsCounter{0};
            int resetOrthonormalBasisCounter{0};
    };

}
