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
#include <sumire/core/profiling/gpu_profiler.hpp>

#include <sumire/core/render_systems/data_structs/high_quality_shadow_mapper_structs.hpp>

#include <sumire/input/sumi_kbm_controller.hpp>

namespace sumire {

    struct SceneViewerDrawData {
        FrameInfo& frameInfo;
        SumiKBMcontroller& cameraController;
        const structs::zBin& zBinData;
        const structs::lightMask* lightMask;
    };

    class SumiImgui {
        public:
            SumiImgui(
                SumiDevice& device,
                SumiConfig& config,
                SumiRenderer& renderer,
                VkRenderPass renderPass,
                uint32_t subpassIdx,
                VkQueue workQueue
            );
            ~SumiImgui();

            SumiImgui(const SumiImgui&) = delete;
            SumiImgui& operator=(const SumiImgui&) = delete;

            void beginFrame();
            void endFrame();
            void render(VkCommandBuffer &buffer);

            void drawSceneViewer(
                FrameInfo &frameInfo, 
                SumiKBMcontroller &cameraController,
                const structs::zBin& zBin,
                structs::lightMask* lightMask,
                GpuProfiler* profiler
            );

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

            void drawConfigSection(SumiKBMcontroller &cameraController);
            void drawConfigGraphicsSubsection();
            void drawConfigInputSubsection(SumiKBMcontroller& cameraController);
            void drawConfigUIsubsection();

            void drawSceneSection(FrameInfo &frameInfo);
            void drawTransformUI(Transform3DComponent &transform, bool includeScale = true);

            void drawProfilingSection(FrameInfo& frameInfo, GpuProfiler* profiler);
            void drawDebugSection(
                const structs::zBin& zBin, structs::lightMask* lightMask);

            void drawHighQualityShadowMappingSection(
                const structs::zBin& zBin, structs::lightMask* lightMask);
            void drawZbinSubsection(const structs::zBin& zbin);
            void drawLightMaskSubsection(structs::lightMask* lightMask);

            SumiConfig& sumiConfig;
            SumiDevice& sumiDevice;
            SumiRenderer& sumiRenderer;
            VkDescriptorPool imguiDescriptorPool;

            int resetProjParamsCounter{0};
            int resetOrthonormalBasisCounter{0};

            static constexpr uint32_t PROFILING_MAX_LINE_PLOT_POINTS = 500u;

            std::array<float, PROFILING_MAX_LINE_PLOT_POINTS> cpuLineGraphPoints{ 0.0 };
            std::array<float, PROFILING_MAX_LINE_PLOT_POINTS> gpuLineGraphPoints{ 0.0 };
    };

}
