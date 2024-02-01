#pragma once

#include "sumi_object.hpp"
#include "sumi_window.hpp"

namespace sumire {

    class SumiKBMcontroller {
    public:
        struct WalkKeybinds {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
        };

        void moveWalk(GLFWwindow* window, float dt, SumiObject& object);

        WalkKeybinds keybinds{};
        float moveSensitivity{3.0f};
        float lookSensitivity{1.5f};
    };
}