#pragma once

#include <sumire/core/sumi_object.hpp>
#include <sumire/core/sumi_window.hpp>

namespace sumire {

    class SumiKBMcontroller {
    public:
        enum ControllerType{ 
            WALK = 0, 
            FPS = 1 
        };

        SumiKBMcontroller(SumiWindow &sumiWindow, ControllerType type);

        struct WalkKeybinds {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_CONTROL;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
            int sprint = GLFW_KEY_LEFT_SHIFT;
            int toggleCursor = GLFW_KEY_LEFT_ALT;
        } keybinds;

        void move(
            float dt, 
            const SumiWindow::KeypressEvents &keypressEvents,
            const glm::vec2 &mouseDelta,
            const glm::mat4 &view,
            Transform3DComponent& transform
        );

        float moveSensitivity{5.0f};
        float sprintSensitivity{2.5f * moveSensitivity};
        
        float keyboardLookSensitivity{2.0f};
        float mouseLookSensitivity{7.0f};
        float cursorHidden{true};
        bool toggleShowCursor{true}; // hold to show cursor by default.

        float dtNoUpdateThreshold{1.0f}; // seconds

        ControllerType getControllerType() const { return type; }
        void setControllerType(ControllerType type) { this->type = type; }

    private:
        void moveWalk(
            float dt, 
            const glm::mat4 &view,
            Transform3DComponent& transform
        );
        void moveFPS(
            float dt, 
            const SumiWindow::KeypressEvents &keypressEvents,
            const glm::vec2 &mouseDelta,
            const glm::mat4 &view,
            Transform3DComponent& transform
        );

        ControllerType type{WALK};
        SumiWindow &sumiWindow;
    };
}