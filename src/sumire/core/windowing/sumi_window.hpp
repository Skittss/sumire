#pragma once

#include <sumire/core/windowing/sumi_key_input.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

namespace sumire {

    class SumiWindow {

    public:
        SumiWindow(int w, int h, std::string name);
        ~SumiWindow();

        SumiWindow(const SumiWindow&) = delete;
        SumiWindow& operator=(const SumiWindow&) = delete;

        VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
        bool wasWindowResized() { return fbResizeFlag; }
        void resetWindowResizedFlag() { fbResizeFlag = false; }
        GLFWwindow *getGLFWwindow() const { return window; }

        // --- GLFW API abstractions ---
        bool shouldClose() { return glfwWindowShouldClose(window); }
        inline static void waitEvents() { glfwWaitEvents(); }
        inline static void pollEvents() { glfwPollEvents(); }
        SumiKeyState getKey(SumiKey key) { return glfwGetKey(window, key); }

        // --- Vulkan related ---
        void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
        static const char** getRequiredInstanceExtensions(uint32_t* count) { return glfwGetRequiredInstanceExtensions(count);}
        
        void showCursor();
        void disableCursor();
        bool isCursorHidden() const { return cursorHidden; }

        glm::vec2 getDimensions() const { return glm::vec2{width, height}; }

        // Mouse Information

        // Whether to poll mouse information with event callbacks or with manual calls to 
        //  SumiWindow::getMousePos() and SumiWindow::setMousePos() via pollMousePos();
        enum MousePollMode{ MANUAL, EVENT_BASED };
        void setMousePollMode(MousePollMode pollMode);
        void pollMousePos();
        void getMousePos(glm::tvec2<double> &pos) { glfwGetCursorPos(window, &pos.x, &pos.y); }
        void setMousePos(glm::tvec2<double> pos) { glfwSetCursorPos(window, pos.x, pos.y); }

        glm::tvec2<double> mouseDelta{0.0f};
        glm::tvec2<double> mousePos{0.0f};
        glm::tvec2<double> prevMousePos{width / 2.0f, height / 2.0f};
        void clearMouseDelta() { mouseDelta = glm::vec2{0.0f}; }

        // Keypress Information
        struct KeypressInfo {
            int scancode;
            int action;
            int mods;
        };
        using KeypressEvents = std::unordered_map<int, KeypressInfo>;
        KeypressEvents keypressEvents;
        void clearKeypressEvents() { keypressEvents.clear(); }

    private:
        MousePollMode mousePollMode; // default poll mode is initialized in constructor

        static void fbResizeCallback(GLFWwindow* window, int width, int height);
        static void mousePosCallback(GLFWwindow* window, double xpos, double ypos);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        
        void initWindow();

        int width;
        int height;
        bool fbResizeFlag = false;

        bool cursorHidden = false;

        std::string windowName;

        GLFWwindow* window;

    };

}