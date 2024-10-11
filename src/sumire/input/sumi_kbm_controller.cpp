#include <sumire/input/sumi_kbm_controller.hpp>

#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace sumire {

    SumiKBMcontroller::SumiKBMcontroller(
        SumiConfig& config,
        SumiWindow& sumiWindow, 
        ControllerType type
    ) : sumiConfig{ config },
        sumiWindow { sumiWindow },
        type{ type } 
    {
        switch(type) {
            // Init cursor behaviour for camera types
            case ControllerType::FPS: {
                if (cursorHidden) sumiWindow.disableCursor();
                else sumiWindow.showCursor();
            }
            break;
            // Default is normal camera behaviour.
            case ControllerType::WALK:
            default: {
                sumiWindow.showCursor();
            }
            break;
        }
    }

    void SumiKBMcontroller::move(
        float dt,
        const SumiWindow::KeypressEvents &keypressEvents,
        const glm::tvec2<double> &mouseDelta,
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
        const auto& keybinds = sumiConfig.runtimeData.keybinds.debugCamera;

        glm::vec3 rotate{0.0f};
        if (sumiWindow.getKey(keybinds.LOOK_RIGHT) == SUMI_KEYSTATE_PRESS) rotate.y -= 1.0f;
        if (sumiWindow.getKey(keybinds.LOOK_LEFT ) == SUMI_KEYSTATE_PRESS) rotate.y += 1.0f;
        if (sumiWindow.getKey(keybinds.LOOK_UP   ) == SUMI_KEYSTATE_PRESS) rotate.x += 1.0f;
        if (sumiWindow.getKey(keybinds.LOOK_DOWN ) == SUMI_KEYSTATE_PRESS) rotate.x -= 1.0f;

        glm::vec3 newRotation = transform.getRotation();
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            newRotation += keyboardLookSensitivity * KBM_SENSITIVITY_FACTOR * glm::normalize(rotate);
        }

        newRotation.x = glm::clamp(newRotation.x, -1.5f, 1.5f);
        newRotation.y = glm::mod(newRotation.y, glm::two_pi<float>());

        transform.setRotation(newRotation);

        float yaw = newRotation.y;
        const glm::vec3 forward = glm::vec3{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, 1.0f, 0.0f}; 

        glm::vec3 moveDir{0.0f};
        if (sumiWindow.getKey(keybinds.MOVE_FORWARD ) == SUMI_KEYSTATE_PRESS) moveDir -= forward;
        if (sumiWindow.getKey(keybinds.MOVE_BACKWARD) == SUMI_KEYSTATE_PRESS) moveDir += forward;
        if (sumiWindow.getKey(keybinds.MOVE_RIGHT   ) == SUMI_KEYSTATE_PRESS) moveDir += right;
        if (sumiWindow.getKey(keybinds.MOVE_LEFT    ) == SUMI_KEYSTATE_PRESS) moveDir -= right;
        if (sumiWindow.getKey(keybinds.MOVE_UP      ) == SUMI_KEYSTATE_PRESS) moveDir += up;
        if (sumiWindow.getKey(keybinds.MOVE_DOWN    ) == SUMI_KEYSTATE_PRESS) moveDir -= up;
        
        const bool sprinting = (sumiWindow.getKey(keybinds.SPRINT) == SUMI_KEYSTATE_PRESS);
        float moveSpeed = (sprinting) ? sprintSensitivity : moveSensitivity;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.setTranslation(transform.getTranslation() + moveSpeed * dt * glm::normalize(moveDir));
        }
    }

    void SumiKBMcontroller::moveFPS(
        float dt, 
        const SumiWindow::KeypressEvents &keypressEvents,
        const glm::tvec2<double> &mouseDelta,
        const glm::mat4 &view,
        Transform3DComponent &transform
    ) {
        const auto& keybinds = sumiConfig.runtimeData.keybinds.debugCamera;

        // Toggle cursor
        auto toggleCursorEvent = keypressEvents.find(keybinds.TOGGLE_CURSOR);
        if (toggleCursorEvent != keypressEvents.end()) {
            switch (toggleCursorEvent->second.action) {
                case SUMI_KEYSTATE_PRESS: {
                    cursorHidden = toggleShowCursor ? !cursorHidden : false;
                }
                break;
                case SUMI_KEYSTATE_RELEASE: {
                    if (!toggleShowCursor) cursorHidden = true;
                }
                break;
                default:
                break;
            }

            // Show / Hide cursor accordingly
            if (cursorHidden) sumiWindow.disableCursor();
            else sumiWindow.showCursor();
        }

        glm::vec3 rotate = cursorHidden ? glm::vec3{-mouseDelta.y, -mouseDelta.x, 0.0f} : glm::vec3{0.0f};

        glm::vec3 newRotation = transform.getRotation();
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            newRotation += mouseLookSensitivity * KBM_SENSITIVITY_FACTOR * rotate;
        }

        newRotation.x = glm::clamp(newRotation.x, -1.5f, 1.5f);
        newRotation.y = glm::mod(newRotation.y, glm::two_pi<float>());

        transform.setRotation(newRotation);

        float yaw = newRotation.y;
        const glm::vec3 forward = glm::vec3{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, 1.0f, 0.0f}; 

        glm::vec3 moveDir{0.0f};
        if (sumiWindow.getKey(keybinds.MOVE_FORWARD ) == SUMI_KEYSTATE_PRESS) moveDir -= forward;
        if (sumiWindow.getKey(keybinds.MOVE_BACKWARD) == SUMI_KEYSTATE_PRESS) moveDir += forward;
        if (sumiWindow.getKey(keybinds.MOVE_RIGHT   ) == SUMI_KEYSTATE_PRESS) moveDir += right;
        if (sumiWindow.getKey(keybinds.MOVE_LEFT    ) == SUMI_KEYSTATE_PRESS) moveDir -= right;
        if (sumiWindow.getKey(keybinds.MOVE_UP      ) == SUMI_KEYSTATE_PRESS) moveDir += up;
        if (sumiWindow.getKey(keybinds.MOVE_DOWN    ) == SUMI_KEYSTATE_PRESS) moveDir -= up;
        
        const bool sprinting = (sumiWindow.getKey(keybinds.SPRINT) == SUMI_KEYSTATE_PRESS);
        float moveSpeed = (sprinting) ? sprintSensitivity : moveSensitivity;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.setTranslation(transform.getTranslation() + moveSpeed * dt * glm::normalize(moveDir));
        }
    }

}