#include <sumire/gui/sumi_imgui.hpp>

#include <sumire/util/vk_check_success.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>

namespace sumire {

    SumiImgui::SumiImgui(
        SumiRenderer &renderer
    ) : sumiRenderer{ renderer } {
        initImgui();
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

    void SumiImgui::initImgui() {

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
        initInfo.Queue = sumiRenderer.getDevice().graphicsQueue();
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.MinImageCount = 2; // double buffer
        initInfo.ImageCount =  SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.UseDynamicRendering = false;
        initInfo.ColorAttachmentFormat = sumiRenderer.getSwapChainImageFormat();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo, sumiRenderer.getSwapChainRenderPass());

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
    
    void SumiImgui::drawStatWindow(FrameInfo &frameInfo, SumiKBMcontroller &cameraController) {
        // ImGui::ShowDemoWindow();
        
        ImGui::Begin("Sumire Scene Viewer");
        ImGui::Text("Sumire Build v0.0.1");

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

        ImGui::Spacing();
        drawConfigUI(cameraController);

        ImGui::Spacing();
        drawSceneUI(frameInfo);

        //ImGui::Text("FPS: %.1f", 1.0);
        ImGui::End();
    }

    void SumiImgui::drawConfigUI(SumiKBMcontroller &cameraController) {
        if (ImGui::CollapsingHeader("Config")) {

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
                const char* controlTypes[] = {"Walk", "FPS"};
                static int controlTypesIdx = cameraController.getControllerType();
                ImGui::Combo("Type", &controlTypesIdx, controlTypes, IM_ARRAYSIZE(controlTypes));
                SumiKBMcontroller::ControllerType controlType = static_cast<SumiKBMcontroller::ControllerType>(controlTypesIdx);
                cameraController.setControllerType(controlType);

                ImGui::Spacing();
                switch(controlType) {
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
            ImGui::Spacing();
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
    }

    void SumiImgui::drawSceneUI(FrameInfo &frameInfo) {
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

                ImGui::Text("Matrix needs update? : %s", frameInfo.camera.projMatrixNeedsUpdate ? "true" : "false");

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

                } else {
                    ImGui::Text("Oopsie - Something went wrong! Please reload.");
                }

                ImGui::Spacing();
                
                // Near and Far planes
                float tNear = frameInfo.camera.near();
                ImGui::DragFloat("Near", &tNear, 0.01f, 0.01f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
                frameInfo.camera.setNear(tNear);

                float tFar = frameInfo.camera.far();
                ImGui::DragFloat("Far", &tFar, 1.0f, 1.0f, 10000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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
                    const char *nodeStrId = std::to_string(kv.first).c_str();
                    if (ImGui::TreeNode(nodeStrId, obj.model->displayName.c_str())) {
                        // Transform
                        drawTransformUI(obj.transform);

                        ImGui::TreePop();
                    }
                }

                ImGui::PopItemWidth();

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Lights")) {
                ImGui::TreePop();
            }
        }
    }

    void SumiImgui::drawTransformUI(Transform3DComponent &transform, bool includeScale) {
        ImGui::SeparatorText("Transform");

        glm::vec3 translation = transform.getTranslation();
        float pos[3] = {
            translation.x, 
            translation.y, 
            translation.z
        };
        ImGui::InputFloat3("translation", pos);
        transform.setTranslation(glm::vec3{pos[0], pos[1], pos[2]});

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
                transform.setScale(glm::vec3{sca[0], sca[1], sca[2]});

            } else {

                float sca = (scale.x + scale.y + scale.z) / 3.0;
                ImGui::DragFloat("scale", &sca, 0.1f);
                transform.setScale(glm::vec3{sca});

            }
        }

        ImGui::Spacing();
    }

}