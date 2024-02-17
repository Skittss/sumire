#include <sumire/gui/sumi_imgui.hpp>

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

        if (vkCreateDescriptorPool(sumiRenderer.getDevice().device(), &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool for ImGui");
        }

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
    
    void SumiImgui::drawStatWindow(FrameInfo &frameInfo) {
        ImGui::ShowDemoWindow();
        ImGui::Begin("Sumire Scene Viewer");
        ImGui::Text("Sumire Build v0.0.1");
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::TreeNode("Camera")) {
                
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
                // Transform UI
                drawTransformUI(frameInfo.camera.transform);
                
                ImGui::SeparatorText("Projection");

                // Projection Type
                const char* projTypes[] = {"Perspective", "Orthographic"};
                static int projTypeIdx = 0;
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

                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                if (ImGui::Button("Reset")) resetProjParamsCounter++;
                if (resetProjParamsCounter > 0) {
                    frameInfo.camera.setDefaultProjectionParams(sumiRenderer.getAspect());
                    resetProjParamsCounter = 0;
                }
                ImGui::PopStyleColor(3);

                frameInfo.camera.calculateProjectionMatrix();

                ImGui::Spacing();
                ImGui::PopItemWidth();

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Objects")) {
                for (auto& kv : frameInfo.objects) {
                    auto& obj = kv.second;
                    const char *nodeStrId = std::to_string(kv.first).c_str();
                    if (ImGui::TreeNode(nodeStrId, obj.model->displayName.c_str())) {
                        // Transform
                        drawTransformUI(obj.transform);

                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Lights")) {
                ImGui::TreePop();
            }

        }
        //ImGui::Text("FPS: %.1f", 1.0);
        ImGui::End();
    }

    void SumiImgui::drawTransformUI(Transform3DComponent &transform) {
        ImGui::SeparatorText("Transform");

        float pos[3] = {
            transform.translation.x, 
            transform.translation.y, 
            transform.translation.z
        };
        ImGui::InputFloat3("translation", pos);
        transform.translation = {pos[0], pos[1], pos[2]};

        float rot[3] = {
            glm::degrees(transform.rotation.x), 
            glm::degrees(transform.rotation.y), 
            glm::degrees(transform.rotation.z)
        };
        ImGui::InputFloat3("rotation (deg)", rot, "%.1f");
        transform.rotation = {
            glm::radians(rot[0]), 
            glm::radians(rot[1]), 
            glm::radians(rot[2])
        };

        ImGui::Spacing();
    }

}