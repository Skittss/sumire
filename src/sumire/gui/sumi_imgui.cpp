#include <sumire/gui/sumi_imgui.hpp>

#include <sumire/util/vk_check_success.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>

namespace sumire {

    SumiImgui::SumiImgui(
        SumiDevice& device,
        SumiConfig& config,
        SumiRenderer &renderer,
        VkRenderPass renderPass,
        uint32_t subpassIdx,
        VkQueue workQueue
    ) : sumiDevice{ device }, sumiConfig{ config }, sumiRenderer { renderer } 
    {
        initImgui(renderPass, subpassIdx, workQueue);
    }

    SumiImgui::~SumiImgui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(sumiDevice.device(), imguiDescriptorPool, nullptr);
    }

    ImGuiIO& SumiImgui::getIO() { return ImGui::GetIO(); }

    GridRendersys::GridUBOdata SumiImgui::getGridUboData() {
        GridRendersys::GridUBOdata ubo{};
        ubo.opacity = gridOpacity;
        ubo.tileSize = gridTileSize;
        ubo.fogNear = gridFogNear;
        ubo.fogFar = gridFogFar;
        ubo.minorLineCol = glm::vec3{gridMinorLineCol.x, gridMinorLineCol.y, gridMinorLineCol.z};
        ubo.xCol = glm::vec3{gridXaxisCol.x, gridXaxisCol.y, gridXaxisCol.z};
        ubo.zCol = glm::vec3{gridZaxisCol.x, gridZaxisCol.y, gridZaxisCol.z};
        
        return ubo;
    }

    void SumiImgui::initImgui(VkRenderPass renderPass, uint32_t subpassIdx, VkQueue workQueue) {

        // Create descriptor pool for ImGui
        VkDescriptorPoolSize poolSizes[] = { 
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } 
        };

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        VK_CHECK_SUCCESS(
            vkCreateDescriptorPool(sumiDevice.device(), &poolInfo, nullptr, &imguiDescriptorPool),
            "[Sumire::SumiImgui] Failed to create ImGui's required descriptor pool."
        );

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = getIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui_ImplGlfw_InitForVulkan(sumiRenderer.getWindow().getGLFWwindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = sumiDevice.getInstance();
        initInfo.PhysicalDevice = sumiDevice.getPhysicalDevice();
        initInfo.Device = sumiDevice.device();
        initInfo.Queue = workQueue;
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.MinImageCount = 2; // double buffer
        initInfo.ImageCount =  SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.UseDynamicRendering = false;
        //initInfo.ColorAttachmentFormat = sumiRenderer.getSwapChainColorFormat();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.RenderPass = renderPass;
        initInfo.Subpass = subpassIdx;

        ImGui_ImplVulkan_Init(&initInfo);

        // TODO: 1. Upload imgui font textures to GPU
        //       2. Then clear from CPU memory
    }

    void SumiImgui::beginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void SumiImgui::endFrame() {
        ImGui::Render();
    }

    void SumiImgui::render(VkCommandBuffer &buffer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
    }
    
    void SumiImgui::drawSceneViewer(
        FrameInfo &frameInfo, 
        SumiKBMcontroller &cameraController,
        const structs::zBin& zBin,
        structs::lightMask* lightMask,
        GpuProfiler* gpuProfiler,
        CpuProfiler* cpuProfiler
    ) {
        //ImGui::ShowDemoWindow();
        
        ImGui::Begin("Sumire Scene Viewer");
        ImGui::Text("Sumire Build v0.0.1");

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

        ImGui::Spacing();
        drawConfigSection(cameraController);

        ImGui::Spacing();
        drawProfilingSection(frameInfo, gpuProfiler, cpuProfiler);

        ImGui::Spacing();
        drawDebugSection(zBin, lightMask);

        ImGui::Spacing();
        drawSceneSection(frameInfo);

        ImGui::End();
    }

    void SumiImgui::drawConfigSection(SumiKBMcontroller& cameraController) {
        if (ImGui::CollapsingHeader("Config")) {

            drawConfigGraphicsSubsection();
            ImGui::Spacing();
            drawConfigInputSubsection(cameraController);
            ImGui::Spacing();
            drawConfigUIsubsection();

        }
    }

    void SumiImgui::drawConfigGraphicsSubsection() {
        // TODO: If we had some menu system for this it would be better to write the graphics settings
        //       Once on menu close rather than on field change.
        ImGui::SeparatorText("Graphics");
        if (ImGui::TreeNode("Settings")) {
            // --------- Device Selection ------------------------------------------------------------
            PhysicalDeviceDetails activeDeviceDetails = sumiDevice.getPhysicalDeviceDetails();
            const std::vector<PhysicalDeviceDetails>& deviceList = sumiDevice.getPhysicalDeviceList();

            std::vector<const char*> deviceNames(deviceList.size());
            for (uint32_t i = 0; i < deviceList.size(); i++) {
                deviceNames[i] = deviceList[i].name.c_str();
            }

            static int deviceListIdx = activeDeviceDetails.idx;
            ImGui::Combo("Device", &deviceListIdx, deviceNames.data(), deviceNames.size());

            uint32_t selectedDeviceIdx = static_cast<uint32_t>(deviceListIdx);
            if (selectedDeviceIdx != sumiConfig.runtimeData.GRAPHICS_DEVICE.IDX) {
                sumiConfig.runtimeData.GRAPHICS_DEVICE = {
                    selectedDeviceIdx,
                    deviceList[selectedDeviceIdx].name.c_str(),
                };
                sumiConfig.writeConfig();
            }

            if (selectedDeviceIdx != activeDeviceDetails.idx) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("A restart is required to change the active graphics device.");
                ImGui::PopStyleColor();
            }

            // --------- Vsync -----------------------------------------------------------------------
            static int vsyncIdx = sumiConfig.runtimeData.VSYNC ? 0 : 1;
            ImGui::Combo("Vsync", &vsyncIdx, "On\0Off\0\0");
            bool vsync = vsyncIdx == 0 ? true : false;
            if (vsync != sumiConfig.runtimeData.VSYNC) {
                sumiConfig.runtimeData.VSYNC = vsync;
                sumiConfig.writeConfig();

                // Update swapchain present mode
                bool test1 = sumiConfig.runtimeData.VSYNC;
                bool test2 = sumiRenderer.getSwapChain()->isVsyncEnabled();
                if (sumiConfig.runtimeData.VSYNC != sumiRenderer.getSwapChain()->isVsyncEnabled()) {
                    sumiRenderer.changeSwapChainPresentMode(sumiConfig.runtimeData.VSYNC);
                }
            }

            ImGui::Spacing();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Shaders")) {
            static bool shaderHotReloadingEnabled = sumiConfig.runtimeData.SHADER_HOT_RELOADING;
            ImGui::Checkbox("Enable Shader Hot-Reloading", &shaderHotReloadingEnabled);

            if (shaderHotReloadingEnabled != sumiConfig.runtimeData.SHADER_HOT_RELOADING) {
                sumiConfig.runtimeData.SHADER_HOT_RELOADING = shaderHotReloadingEnabled;
                sumiConfig.writeConfig();
            }

            if (shaderHotReloadingEnabled != sumiConfig.startupData.SHADER_HOT_RELOADING) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("A restart is required to change hot reloading settings.");
                ImGui::PopStyleColor();
            }

            ImGui::TreePop();
        }
    }

    void SumiImgui::drawConfigInputSubsection(SumiKBMcontroller &cameraController) {
        ImGui::SeparatorText("Input");
        if (ImGui::TreeNode("Mouse")) {
            glm::vec2 mousePos = sumiRenderer.getWindow().mousePos;
            ImGui::Text("Mouse pos: %.2f, %.2f", mousePos.x, mousePos.y);
            glm::vec2 mouseDelta = sumiRenderer.getWindow().mouseDelta;
            ImGui::Text("Mouse delta: %.2f, %.2f", mouseDelta.x, mouseDelta.y);
            ImGui::Spacing();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Keyboard")) {
            ImGui::Text("Todo");
            ImGui::Spacing();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Camera Controls")) {
            const char* controlTypes[] = { "Walk", "FPS" };
            static int controlTypesIdx = cameraController.getControllerType();
            ImGui::Combo("Type", &controlTypesIdx, controlTypes, IM_ARRAYSIZE(controlTypes));
            SumiKBMcontroller::ControllerType controlType = static_cast<SumiKBMcontroller::ControllerType>(controlTypesIdx);
            cameraController.setControllerType(controlType);

            ImGui::Spacing();
            switch (controlType) {
            case SumiKBMcontroller::ControllerType::FPS: {
                ImGui::Checkbox("Toggle show cursor", &cameraController.toggleShowCursor);
                ImGui::Spacing();
                ImGui::DragFloat("Mouse Sensitivity", &cameraController.mouseLookSensitivity, 0.01f, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            }
            break;
            case SumiKBMcontroller::ControllerType::WALK: {
                ImGui::DragFloat("Look Sensitivity", &cameraController.keyboardLookSensitivity, 0.01f, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            }
            break;
            }
            ImGui::Spacing();
            ImGui::DragFloat("Move Speed", &cameraController.moveSensitivity, 0.1f, 0.0f, 20.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Sprint Speed", &cameraController.sprintSensitivity, 0.1f, 0.0f, 20.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::Spacing();
            ImGui::TreePop();
        }
    }

    void SumiImgui::drawConfigUIsubsection() {
        ImGui::SeparatorText("UI");
        if (ImGui::TreeNode("Grid")) {
            ImGui::SeparatorText("Visibility");
            ImGui::Checkbox("Show", &showGrid);
            ImGui::DragFloat("Opacity", &gridOpacity, 0.01f, 0.0, 1.0, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::Spacing();
            //ImGui::DragFloat("Fog Near", &gridFogNear, 0.1f, 0.0, 100.0, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Fog Far", &gridFogFar, 0.1f, 0.0, 100.0, "%.2f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::Spacing();
            ImGui::SeparatorText("Size");
            ImGui::DragFloat("Tile size", &gridTileSize, 1.0f, 0.0, 50.0, "%.1f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::Spacing();
            ImGui::SeparatorText("Gridline Colours");
            const ImGuiColorEditFlags colorPickerFlags = ImGuiColorEditFlags_AlphaPreview;
            ImGui::ColorEdit3("Minor", (float*)&gridMinorLineCol, colorPickerFlags);
            ImGui::ColorEdit3("X Axis", (float*)&gridXaxisCol, colorPickerFlags);
            ImGui::ColorEdit3("Z Axis", (float*)&gridZaxisCol, colorPickerFlags);
            ImGui::TreePop();
        }
    }

    void SumiImgui::drawSceneSection(FrameInfo &frameInfo) {
        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::TreeNode("Camera")) {
                
                // Transform UI
                drawTransformUI(frameInfo.camera.transform, false);
                
                // Orthonormal Basis
                ImGui::SeparatorText("Orthonormal Basis");

                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                if (ImGui::Button("Reset Basis")) resetOrthonormalBasisCounter++;
                if (resetOrthonormalBasisCounter > 0) {
                    frameInfo.camera.setDefaultOrthonormalBasis();
                    resetOrthonormalBasisCounter = 0;
                }
                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                glm::vec3 camRight = frameInfo.camera.getBasisRight();
                float right[3] {camRight.x, camRight.y, camRight.z};
                ImGui::InputFloat3("right", right, "%.2f");
                frameInfo.camera.setBasisRight(glm::vec3{right[0], right[1], right[2]});

                glm::vec3 camUp = frameInfo.camera.getBasisUp();
                float up[3] {camUp.x, camUp.y, camUp.z};
                ImGui::InputFloat3("up", up, "%.2f");
                frameInfo.camera.setBasisUp(glm::vec3{up[0], up[1], up[2]});

                glm::vec3 camForward = frameInfo.camera.getBasisForward();
                float forward[3] {camForward.x, camForward.y, camForward.z};
                ImGui::InputFloat3("forward", forward, "%.2f");
                frameInfo.camera.setBasisForward(glm::vec3{forward[0], forward[1], forward[2]});

                ImGui::Spacing();

                ImGui::SeparatorText("Projection");

                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                if (ImGui::Button("Reset Projection Params")) resetProjParamsCounter++;
                if (resetProjParamsCounter > 0) {
                    frameInfo.camera.setDefaultProjectionParams(sumiRenderer.getAspect());
                    resetProjParamsCounter = 0;
                }
                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                // Projection Type
                const char* projTypes[] = {"Perspective", "Orthographic"};
                static int projTypeIdx = frameInfo.camera.getCameraType();
                ImGui::Combo("Type", &projTypeIdx, projTypes, IM_ARRAYSIZE(projTypes));
                frameInfo.camera.setCameraType(static_cast<SmCameraType>(projTypeIdx));

                ImGui::Spacing();

                if (projTypeIdx == 0) {
                    // Fov Slider
                    float varFovy = glm::degrees(frameInfo.camera.getFovy());
                    ImGui::DragFloat("FOV (y)", &varFovy, 1.0f, 0.01f, 179.99f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                    frameInfo.camera.setFovy(glm::radians(varFovy));

                    // Aspect input
                    // TODO: This should be based on a multiple of the current aspect ratio so the stretch factor stays
                    //        When window size is changed?
                    float varAspect = frameInfo.camera.getAspect();
                    ImGui::InputFloat("Aspect", &varAspect);
                    frameInfo.camera.setAspect(varAspect);

                } else if (projTypeIdx == 1) {
                    float varOrthoL = frameInfo.camera.getOrthoLeft();
                    ImGui::InputFloat("Left", &varOrthoL);
                    frameInfo.camera.setOrthoLeft(varOrthoL);
                    
                    float varOrthoR = frameInfo.camera.getOrthoRight();
                    ImGui::InputFloat("Right", &varOrthoR);
                    frameInfo.camera.setOrthoRight(varOrthoR);

                    float varOrthoT = frameInfo.camera.getOrthoTop();
                    ImGui::InputFloat("Top", &varOrthoT);
                    frameInfo.camera.setOrthoTop(varOrthoT);

                    float varOrthoB = frameInfo.camera.getOrthoBot();
                    ImGui::InputFloat("Bottom", &varOrthoB);
                    frameInfo.camera.setOrthoBot(varOrthoB);

                    float varOrthoZoom = frameInfo.camera.getOrthoZoom();
                    ImGui::DragFloat("Zoom", &varOrthoZoom, 0.01f, 0.01f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                    frameInfo.camera.setOrthoZoom(varOrthoZoom);

                } else {
                    ImGui::Text("Oopsie - Something went wrong! Please reload.");
                }

                ImGui::Spacing();
                
                // Near and Far planes
                float tNear = frameInfo.camera.getNear();
                ImGui::DragFloat("Near", &tNear, 0.01f, 0.00f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                frameInfo.camera.setNear(tNear);

                float tFar = frameInfo.camera.getFar();
                ImGui::DragFloat("Far", &tFar, 1.0f, 0.01f, 10000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                frameInfo.camera.setFar(tFar);

                frameInfo.camera.calculateProjectionMatrix();

                ImGui::Spacing();
                ImGui::PopItemWidth();

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Objects")) {

                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
                for (auto& kv : frameInfo.objects) {
                    auto& obj = kv.second;
                    const std::string nodeStrId = std::to_string(kv.first);
                    const char* nodeCharId = nodeStrId.c_str();
                    if (ImGui::TreeNode(nodeCharId, obj.model->displayName.c_str())) {
                        // Transform
                        drawTransformUI(obj.transform);

                        ImGui::TreePop();
                    }
                }
                ImGui::PopItemWidth();

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Lights")) {
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
                for (auto& kv : frameInfo.lights) {
                    auto& light = kv.second;
                    const std::string nodeStrId = std::to_string(kv.first);
                    const char* nodeCharId = nodeStrId.c_str();
                    if (ImGui::TreeNode(nodeCharId, light.name.c_str())) {
                        const char* lightTypes[] = {"Point", "Spot", "Directional"};
                        static int lightTypeIdx = frameInfo.camera.getCameraType();
                        ImGui::Combo("Type", &lightTypeIdx, lightTypes, IM_ARRAYSIZE(lightTypes));
                        SumiLight::Type lightType = static_cast<SumiLight::Type>(lightTypeIdx);
                        light.type = lightType;

                        drawTransformUI(light.transform, false);

                        ImGui::SeparatorText("Illumination");
                        const ImGuiColorEditFlags colorPickerFlags = ImGuiColorEditFlags_AlphaPreview;
                        float lightCol[3] = { light.color.r, light.color.g, light.color.b };
                        ImGui::ColorEdit3("Color", lightCol, colorPickerFlags);
                        light.color.r = lightCol[0];
                        light.color.g = lightCol[1];
                        light.color.b = lightCol[2];

                        float intensity = light.color.a;
                        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.00f, 1.0f, "%.2f");
                        light.color.a = intensity;

                        ImGui::Spacing();
                        switch (light.type) {
                            case SumiLight::PUNCTUAL_POINT: {
                                ImGui::SeparatorText("Attenuation");
                                float range = light.range;
                                ImGui::DragFloat("Range", &range, 0.01f, 0.00f, 100.0f, "%.2f");
                                light.range = range;
                            }
                            break;
                            case SumiLight::PUNCTUAL_SPOT: {
                                ImGui::SeparatorText("Attenuation");
                                float range = light.range;
                                ImGui::DragFloat("Range", &range, 0.01f, 0.00f, 100.0f, "%.2f");
                                light.range =  range;

                                ImGui::Spacing();
                                float innerConeAngle = light.innerConeAngle;
                                ImGui::DragFloat("Inner Cone Angle", &innerConeAngle, 0.001f, 0.00f, glm::half_pi<float>(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                light.innerConeAngle = innerConeAngle;

                                float outerConeAngle = light.outerConeAngle;
                                ImGui::DragFloat("Outer Cone Angle", &outerConeAngle, 0.001f, light.innerConeAngle, glm::half_pi<float>(), "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                light.outerConeAngle = outerConeAngle;
                            }
                            break;
                            case SumiLight::PUNCTUAL_DIRECTIONAL: {

                            }
                            break;
                            default: {
                                ImGui::Text("Could not load light parameters!");
                            }
                        }

                        ImGui::TreePop();
                    }

                }
                ImGui::PopItemWidth();

                ImGui::TreePop();
            }
        }
    }

    void SumiImgui::drawTransformUI(Transform3DComponent& transform, bool includeScale) {
        ImGui::SeparatorText("Transform");

        glm::vec3 translation = transform.getTranslation();
        float pos[3] = {
            translation.x,
            translation.y,
            translation.z
        };
        ImGui::InputFloat3("translation", pos);
        transform.setTranslation(glm::vec3{ pos[0], pos[1], pos[2] });

        glm::vec3 rotation = transform.getRotation();
        float rot[3] = {
            glm::degrees(rotation.x),
            glm::degrees(rotation.y),
            glm::degrees(rotation.z)
        };
        ImGui::InputFloat3("rotation (deg)", rot, "%.1f");
        transform.setRotation(glm::vec3{
            glm::radians(rot[0]),
            glm::radians(rot[1]),
            glm::radians(rot[2])
            });

        if (includeScale) {
            ImGui::Spacing();

            static bool perAxisScale = false;
            ImGui::Checkbox("Per-axis scale?", &perAxisScale);

            glm::vec3 scale = transform.getScale();
            if (perAxisScale) {

                float sca[3] = {
                    scale.x,
                    scale.y,
                    scale.z
                };
                ImGui::InputFloat3("scale", sca);
                transform.setScale(glm::vec3{ sca[0], sca[1], sca[2] });

            }
            else {

                float sca = (scale.x + scale.y + scale.z) / 3.0;
                ImGui::DragFloat("scale", &sca, 0.1f);
                transform.setScale(glm::vec3{ sca });

            }
        }

        ImGui::Spacing();
    }

    void SumiImgui::drawProfilingSection(
        FrameInfo& frameInfo, 
        GpuProfiler* gpuProfiler,
        CpuProfiler* cpuProfiler
    ) {
        // TODO: It would be good to rolling average these values so they are more readable.
        if (ImGui::CollapsingHeader("Profiling")) {

            // ---- CPU ------------------------------------------------------------------------------------------
            ImGui::SeparatorText("CPU Profiling");
            static bool cpuProfilingEnabled = sumiConfig.runtimeData.CPU_PROFILING;
            ImGui::Checkbox("Enable CPU profiling blocks", &cpuProfilingEnabled);

            if (cpuProfilingEnabled != sumiConfig.runtimeData.CPU_PROFILING) {
                sumiConfig.runtimeData.CPU_PROFILING = cpuProfilingEnabled;
                sumiConfig.writeConfig();
            }

            if (cpuProfilingEnabled != sumiConfig.startupData.CPU_PROFILING) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("A restart is required to change profiling settings.");
                ImGui::PopStyleColor();
            }

            float fps = frameInfo.frameTime == 0.0f ? 0.0f : 1.0f / frameInfo.frameTime;

            std::rotate(cpuLineGraphPoints.begin(), cpuLineGraphPoints.begin() + 1, cpuLineGraphPoints.end());
            cpuLineGraphPoints.back() = fps;

            ImGui::PlotLines("", cpuLineGraphPoints.data(), cpuLineGraphPoints.size(),
                0, "FPS", 0.0f, 1000.0f, ImVec2(500.0f, 55.0f));

            ImGui::Text("Frame time - %.5f ms (%.1f FPS)", frameInfo.frameTime * 1000.0, fps);
            ImGui::Spacing();

            if (cpuProfiler) {
                for (auto& kv : cpuProfiler->getNamedBlocks()) {
                    ImGui::Text("%.5f ms - %s", kv.second.ms, kv.first.c_str());
                }
                ImGui::Spacing();
            }

            // ---- GPU ------------------------------------------------------------------------------------------
            ImGui::SeparatorText("GPU Profiling");
            static bool gpuProfilingEnabled = sumiConfig.runtimeData.GPU_PROFILING;
            ImGui::Checkbox("Enable GPU profiling", &gpuProfilingEnabled);

            if (gpuProfilingEnabled != sumiConfig.runtimeData.GPU_PROFILING) {
                sumiConfig.runtimeData.GPU_PROFILING = gpuProfilingEnabled;
                sumiConfig.writeConfig();
            }

            if (gpuProfilingEnabled != sumiConfig.startupData.GPU_PROFILING) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("A restart is required to change profiling settings.");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            if (gpuProfiler) {
                double total_ms = 0.0;
                for (auto& kv : gpuProfiler->getNamedBlocks()) {
                    ImGui::Text("%.5f ms - %s", kv.second.ms, kv.first.c_str());
                    total_ms += kv.second.ms;
                }
                ImGui::Spacing();

                std::rotate(gpuLineGraphPoints.begin(), gpuLineGraphPoints.begin() + 1, gpuLineGraphPoints.end());
                gpuLineGraphPoints.back() = total_ms;

                ImGui::PlotLines("", gpuLineGraphPoints.data(), gpuLineGraphPoints.size(),
                    0, "Frame ms", 0.0f, 15.0f, ImVec2(500.0f, 55.0f));

                ImGui::Text("Total - %.5f ms", total_ms);
            }
            else {
                ImGui::Text("No profiler attached.");
            }

            ImGui::Spacing();
        }
    }

    void SumiImgui::drawDebugSection(
        const structs::zBin& zBin,
        structs::lightMask* lightMask
    ) {
        if (ImGui::CollapsingHeader("Debug")) {
            ImGui::SeparatorText("Debug Shaders");
            static bool debuggingShadersEnabled = sumiConfig.runtimeData.DEBUG_SHADERS;
            ImGui::Checkbox("Enable Debug Shaders", &debuggingShadersEnabled);

            if (debuggingShadersEnabled != sumiConfig.runtimeData.DEBUG_SHADERS) {
                sumiConfig.runtimeData.DEBUG_SHADERS = debuggingShadersEnabled;
                sumiConfig.writeConfig();
            }

            if (debuggingShadersEnabled != sumiConfig.startupData.DEBUG_SHADERS) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("A restart is required to change debug shader settings.");
                ImGui::PopStyleColor();
            }

            ImGui::SeparatorText("Systems");
            drawHighQualityShadowMappingSection(zBin, lightMask);

            ImGui::Spacing();
        }

    }

    void SumiImgui::drawHighQualityShadowMappingSection(
        const structs::zBin& zBin,
        structs::lightMask* lightMask
    ) {
        if (ImGui::TreeNode("High Quality Shadow Mapping")) {
            drawZbinSubsection(zBin);
            drawLightMaskSubsection(lightMask);
            drawHqsmDebugViewSubsection();

            ImGui::TreePop();
        }
    }

    void SumiImgui::drawZbinSubsection(const structs::zBin& zBin) {
        if (ImGui::TreeNode("zBin")) {

            uint32_t nBins = static_cast<uint32_t>(zBin.data.size());
            ImGui::Text("zBin size: %u", nBins);
            ImGui::Spacing();
            ImGui::Text("Light index range: [%d, %d]", zBin.minLight, zBin.maxLight);
            ImGui::Text("Full bin index range: [%d, %d]", zBin.firstFullIdx, zBin.lastFullIdx);
            ImGui::Spacing();

            constexpr ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV;

            constexpr int cols = 5;

            const ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10);
            if (ImGui::BeginTable("zBin Data", cols, flags, outer_size)) {
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableSetupColumn("bin",
                    ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("min", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("max", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("rMin", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("rMax", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < zBin.data.size(); i++) {
                    uint32_t idx = static_cast<uint32_t>(i);
                    ImGui::TableNextRow();
                    for (int j = 0; j < cols; j++) {
                        if (!ImGui::TableSetColumnIndex(j) && j > 0)
                            continue;

                        if (j == 0)
                            ImGui::Text("%u", static_cast<uint32_t>(i));
                        else
                            switch (j) {
                            case 0:
                                ImGui::Text("%u", i);
                                break;
                            case 1:
                                ImGui::Text("%d", zBin.data[i].minLightIdx);
                                break;
                            case 2:
                                ImGui::Text("%d", zBin.data[i].maxLightIdx);
                                break;
                            case 3:
                                ImGui::Text("%d", zBin.data[i].rangedMinLightIdx);
                                break;
                            case 4:
                                ImGui::Text("%d", zBin.data[i].rangedMaxLightIdx);
                                break;
                            default:
                                ImGui::Text("OOB!");
                            }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Spacing();
            ImGui::TreePop();
        }
    }

    void SumiImgui::drawLightMaskSubsection(structs::lightMask* lightMask) {
        if (ImGui::TreeNode("Light Mask")) {
            ImGui::Text("Num Tiles: [%u, %u]", lightMask->numTilesX, lightMask->numTilesY);
            ImGui::Spacing();

            ImGui::Text("Currently Inspecting: ");
            static int tileIdx[2]{ 0, 0 };
            ImGui::InputInt2("", tileIdx);
            tileIdx[0] = glm::clamp<int>(tileIdx[0], 0, lightMask->numTilesX - 1);
            tileIdx[1] = glm::clamp<int>(tileIdx[1], 0, lightMask->numTilesY - 1);
            ImGui::Spacing();

            constexpr ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_BordersV |
                ImGuiTableFlags_NoHostExtendX;

            constexpr int tileRows = 33;
            constexpr int tileCols = 32;

            const ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10);
            const ImU32 bitSetColor = ImGui::GetColorU32(ImVec4(0.1f, 0.9f, 0.1f, 0.6f));
            const ImU32 bitNotSetColor = ImGui::GetColorU32(ImVec4(0.9f, 0.1f, 0.1f, 0.6f));

            if (ImGui::BeginTable("Tile Light Mask", tileCols + 1, flags, outer_size)) {
                ImGui::TableSetupScrollFreeze(1, 1);
                for (int c = 0; c < tileCols + 1; c++) {
                    switch (c) {
                    case 0:
                        ImGui::TableSetupColumn("");
                        break;
                    default:
                        ImGui::TableSetupColumn(
                            std::to_string(c - 1).c_str(), ImGuiTableColumnFlags_WidthFixed, 12.0f);
                        break;
                    }
                }
                ImGui::TableHeadersRow();

                for (int row = 0; row < tileRows; row++) {
                    ImGui::TableNextRow();

                    for (int col = 0; col < tileCols + 1; col++) {
                        if (!ImGui::TableSetColumnIndex(col) && col > 0)
                            continue;

                        if (col == 0) {
                            switch (row) {
                            case 0:
                                ImGui::Text("[Groups]");
                                break;
                            default:
                                uint32_t groupIdx = static_cast<uint32_t>(row) - 1u;
                                ImGui::Text("[%u, %u]",
                                    groupIdx * 32, groupIdx * 32 + 31);
                            }
                        }
                        else {
                            uint32_t tileX = static_cast<uint32_t>(tileIdx[0]);
                            uint32_t tileY = static_cast<uint32_t>(tileIdx[1]);
                            if (lightMask->readTileAtIdx(tileX, tileY).isBitSet(col - 1, row)) {
                                ImGui::PushStyleColor(ImGuiCol_Text, bitSetColor);
                                ImGui::Text("1");
                                ImGui::PopStyleColor();
                            }
                            else {
                                ImGui::PushStyleColor(ImGuiCol_Text, bitNotSetColor);
                                ImGui::Text("0");
                                ImGui::PopStyleColor();
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }

    void SumiImgui::drawHqsmDebugViewSubsection() {
        if (ImGui::TreeNode("Debug Views")) {

            if (sumiConfig.runtimeData.DEBUG_SHADERS) {
                // Debug View Selection
                const char* debugViews[] = { "None", "Light Count", "Light Culling" };
                static int debugViewIdx = 0;
                ImGui::Combo("Type", &debugViewIdx, debugViews, IM_ARRAYSIZE(debugViews));
                HQSMdebugView = static_cast<HQSMdebuggerView>(debugViewIdx);

            }
            else {
                ImGui::Text("Debug Shaders are disabled.");
            }

            ImGui::TreePop();

        }
    }

}