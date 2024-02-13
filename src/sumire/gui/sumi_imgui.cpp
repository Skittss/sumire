#include <sumire/gui/sumi_imgui.hpp>

#include <stdexcept>

namespace sumire {

    SumiImgui::SumiImgui(
        SumiDevice &device, const SumiWindow &window, SumiRenderer &renderer
    ) : sumiDevice{ device } {
        initImgui(device, window.getGLFWwindow(), renderer);
    }

    SumiImgui::~SumiImgui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(sumiDevice.device(), imguiDescriptorPool, nullptr);
    }

    ImGuiIO& SumiImgui::getIO() { return ImGui::GetIO(); }

    void SumiImgui::initImgui(SumiDevice &device, GLFWwindow *window, SumiRenderer &renderer) {

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

        if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool for ImGui");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto io = getIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = device.getInstance();
        initInfo.PhysicalDevice = device.getPhysicalDevice();
        initInfo.Device = device.device();
        initInfo.Queue = device.graphicsQueue();
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.MinImageCount = 2; // double buffer
        initInfo.ImageCount =  SumiSwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.UseDynamicRendering = false;
        initInfo.ColorAttachmentFormat = renderer.getSwapChainImageFormat();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo, renderer.getSwapChainRenderPass());

        // TODO: 1. Upload imgui font textures to GPU
        //       2. Then clear from CPU memory
    }

    void SumiImgui::beginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void SumiImgui::drawStatWindow() {
        ImGui::ShowDemoWindow();
        //ImGui::Begin("Statistics", &showStatsWindow);
        //ImGui::Text("Test window");
        //ImGui::Text("FPS: %.1f", 1.0);
        //ImGui::End();
    }

    void SumiImgui::endFrame() {
        ImGui::Render();
    }

    void SumiImgui::renderToCmdBuffer(VkCommandBuffer &buffer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
    }
    
}