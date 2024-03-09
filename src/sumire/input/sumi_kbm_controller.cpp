#include <sumire/input/sumi_kbm_controller.hpp>

#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace sumire {

    SumiKBMcontroller::SumiKBMcontroller(SumiWindow &sumiWindow, ControllerType type) 
        : sumiWindow{ sumiWindow}, type{ type } 
    {
        switch(type) {
            // Init cursor behaviour for camera types
            case ControllerType::FPS: {
                glfwSetInputMode(sumiWindow.getGLFWwindow(), GLFW_CURSOR, cursorHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            }
            break;
            // Default is normal camera behaviour.
            case ControllerType::WALK:
            default: {
                glfwSetInputMode(sumiWindow.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            break;
        }
    }

    void SumiKBMcontroller::move(
        float dt,
        const SumiWindow::KeypressEvents &keypressEvents,
        const glm::vec2 &mouseDelta,
        const glm::mat4 &view,
        Transform3DComponent& transform
    ) {
        // Do not update when lag spikes cause large dt. These updates can be jarring when the program
        //  then resumes.
        if (dt > dtNoUpdateThreshold) return;

        switch(type) {
            case ControllerType::WALK: {
                moveWalk(dt, view, transform);
            }
            break;
            case ControllerType::FPS: {
                moveFPS(dt, keypressEvents, mouseDelta, view, transform);
            }
            break;
        }
    }

    void SumiKBMcontroller::moveWalk(
        float dt, 
        const glm::mat4 &view,
        Transform3DComponent &transform
    ) {
        GLFWwindow *window = sumiWindow.getGLFWwindow();

        glm::vec3 rotate{0.0f};
        if (glfwGetKey(window, keybinds.lookRight) == GLFW_PRESS) rotate.y -= 1.0f;
        if (glfwGetKey(window, keybinds.lookLeft ) == GLFW_PRESS) rotate.y += 1.0f;
        if (glfwGetKey(window, keybinds.lookUp   ) == GLFW_PRESS) rotate.x += 1.0f;
        if (glfwGetKey(window, keybinds.lookDown ) == GLFW_PRESS) rotate.x -= 1.0f;

        glm::vec3 newRotation = transform.getRotation();
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            newRotation += keyboardLookSensitivity * dt * glm::normalize(rotate);
        }

        newRotation.x = glm::clamp(newRotation.x, -1.5f, 1.5f);
        newRotation.y = glm::mod(newRotation.y, glm::two_pi<float>());

        transform.setRotation(newRotation);

        float yaw = newRotation.y;
        const glm::vec3 forward = glm::vec3{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, 1.0f, 0.0f}; 

        glm::vec3 moveDir{0.0f};
        if (glfwGetKey(window, keybinds.moveForward ) == GLFW_PRESS) moveDir -= forward;
        if (glfwGetKey(window, keybinds.moveBackward) == GLFW_PRESS) moveDir += forward;
        if (glfwGetKey(window, keybinds.moveRight   ) == GLFW_PRESS) moveDir += right;
        if (glfwGetKey(window, keybinds.moveLeft    ) == GLFW_PRESS) moveDir -= right;
        if (glfwGetKey(window, keybinds.moveUp      ) == GLFW_PRESS) moveDir += up;
        if (glfwGetKey(window, keybinds.moveDown    ) == GLFW_PRESS) moveDir -= up;
        
        float moveSpeed = (glfwGetKey(window, keybinds.sprint) == GLFW_PRESS) ? sprintSensitivity : moveSensitivity;
        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.setTranslation(transform.getTranslation() + moveSpeed * dt * glm::normalize(moveDir));
        }
    }

    void SumiKBMcontroller::moveFPS(
        float dt, 
        const SumiWindow::KeypressEvents &keypressEvents,
        const glm::vec2 &mouseDelta,
        const glm::mat4 &view,
        Transform3DComponent &transform
    ) {
        GLFWwindow *window = sumiWindow.getGLFWwindow();

        // Toggle cursor
        auto toggleCursorEvent = keypressEvents.find(keybinds.toggleCursor);
        if (toggleCursorEvent != keypressEvents.end()) {
            switch (toggleCursorEvent->second.action) {
                case GLFW_PRESS: {
                    cursorHidden = toggleShowCursor ? !cursorHidden : false;
                }
                break;
                case GLFW_RELEASE: {
                    if (!toggleShowCursor) cursorHidden = true;
                }
                default:
                break;
            }
            // TODO: This functionality should really be moved to the sumiWindow container
            glfwSetInputMode(window, GLFW_CURSOR, cursorHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }

        // TODO: This rotation needs to be done in quaternions as there is some gimbal locking present currently.
        //       This may need to be changed in the camera class itself.
        glm::vec3 rotate = cursorHidden ? glm::vec3{-mouseDelta.y, -mouseDelta.x, 0.0f} : glm::vec3{0.0f};

        glm::vec3 newRotation = transform.getRotation();
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            newRotation += mouseLookSensitivity * dt * glm::normalize(rotate);
        }

        newRotation.x = glm::clamp(newRotation.x, -1.5f, 1.5f);
        newRotation.y = glm::mod(newRotation.y, glm::two_pi<float>());

        transform.setRotation(newRotation);

        float yaw = newRotation.y;
        const glm::vec3 forward = glm::vec3{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, 1.0f, 0.0f}; 

        glm::vec3 moveDir{0.0f};
        if (glfwGetKey(window, keybinds.moveForward ) == GLFW_PRESS) moveDir -= forward;
        if (glfwGetKey(window, keybinds.moveBackward) == GLFW_PRESS) moveDir += forward;
        if (glfwGetKey(window, keybinds.moveRight   ) == GLFW_PRESS) moveDir += right;
        if (glfwGetKey(window, keybinds.moveLeft    ) == GLFW_PRESS) moveDir -= right;
        if (glfwGetKey(window, keybinds.moveUp      ) == GLFW_PRESS) moveDir += up;
        if (glfwGetKey(window, keybinds.moveDown    ) == GLFW_PRESS) moveDir -= up;
        
        float moveSpeed = (glfwGetKey(window, keybinds.sprint) == GLFW_PRESS) ? sprintSensitivity : moveSensitivity;
        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.setTranslation(transform.getTranslation() + moveSpeed * dt * glm::normalize(moveDir));
        }

    }

}