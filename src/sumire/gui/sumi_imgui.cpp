#include <sumire/gui/sumi_imgui.hpp>

#include <sumire/util/vk_check_success.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>

namespace sumire {

    SumiImgui::SumiImgui(
        SumiRenderer &renderer,
        VkRenderPass renderPass,
        uint32_t subpassIdx,
        VkQueue workQueue
    ) : sumiRenderer{ renderer } {
        initImgui(renderPass, subpassIdx, workQueue);
    }

    SumiImgui::~SumiImgui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(sumiRenderer.getDevice().device(), imguiDescriptorPool, nullptr);
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
            vkCreateDescriptorPool(sumiRenderer.getDevice().device(), &poolInfo, nullptr, &imguiDescriptorPool),
            "[Sumire::SumiImgui] Failed to create ImGui's required descriptor pool."
        );

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto io = getIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui_ImplGlfw_InitForVulkan(sumiRenderer.getWindow().getGLFWwindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = sumiRenderer.getDevice().getInstance();
        initInfo.PhysicalDevice = sumiRenderer.getDevice().getPhysicalDevice();
        initInfo.Device = sumiRenderer.getDevice().device();
        initInfo.Queue = workQueue;
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.MinImageCount = 2; // double buffer
        initInfo.ImageCount =  SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.UseDynamicRendering = false;
        initInfo.ColorAttachmentFormat = sumiRenderer.getSwapChainColorFormat();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Subpass = subpassIdx;

        ImGui_ImplVulkan_Init(&initInfo, renderPass);

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

    void SumiImgui::renderToCmdBuffer(VkCommandBuffer &buffer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
    }
    
    void SumiImgui::drawSceneViewer(
        FrameInfo &frameInfo, 
        SumiKBMcontroller &cameraController,
        const structs::zBin& zBin,
        structs::lightMask* lightMask
    ) {
        ImGui::ShowDemoWindow();
        
        ImGui::Begin("Sumire Scene Viewer");
        ImGui::Text("Sumire Build v0.0.1");

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

        ImGui::Spacing();
        drawDebugSection(zBin, lightMask);

        ImGui::Spacing();
        drawConfigSection(cameraController);

        ImGui::Spacing();
        drawSceneSection(frameInfo);

        ImGui::End();
    }

    void SumiImgui::drawConfigSection(SumiKBMcontroller& cameraController) {
        if (ImGui::CollapsingHeader("Config")) {

            drawConfigInputSubsection(cameraController);
            ImGui::Spacing();
            drawConfigUIsubsection();

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

                glm::vec3 camRight = frameInfo.camera.getRight();
                float right[3] {camRight.x, camRight.y, camRight.z};
                ImGui::InputFloat3("right", right, "%.2f");
                frameInfo.camera.setRight(glm::vec3{right[0], right[1], right[2]});

                glm::vec3 camUp = frameInfo.camera.getUp();
                float up[3] {camUp.x, camUp.y, camUp.z};
                ImGui::InputFloat3("up", up, "%.2f");
                frameInfo.camera.setUp(glm::vec3{up[0], up[1], up[2]});

                glm::vec3 camForward = frameInfo.camera.getForward();
                float forward[3] {camForward.x, camForward.y, camForward.z};
                ImGui::InputFloat3("forward", forward, "%.2f");
                frameInfo.camera.setForward(glm::vec3{forward[0], forward[1], forward[2]});

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

    void SumiImgui::drawDebugSection(
        const structs::zBin& zBin,
        structs::lightMask* lightMask
    ) {
        if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawFrameTimingSubsection();
            drawHighQualityShadowMappingSection(zBin, lightMask);

            ImGui::Spacing();
        }

    }

    void SumiImgui::drawFrameTimingSubsection() {
        if (ImGui::TreeNode("Frame Timings")) {
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

            ImGui::TreePop();
        }
    }

    void SumiImgui::drawHighQualityShadowMappingSection(
        const structs::zBin& zBin,
        structs::lightMask* lightMask
    ) {
        if (ImGui::TreeNode("High Quality Shadow Mapping")) {
            drawZbinSubsection(zBin);
            drawLightMaskSubsection(lightMask);

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

}