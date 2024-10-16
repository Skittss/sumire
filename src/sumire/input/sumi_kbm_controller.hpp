#pragma once

#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/windowing/sumi_window.hpp>

// To make input values to a more sensible 0->1 sensitivity range;
#define KBM_SENSITIVITY_FACTOR 0.05f

namespace sumire {

    class SumiKBMcontroller {
    public:
        enum ControllerType{ 
            WALK = 0, 
            FPS = 1 
        };

        SumiKBMcontroller(
            SumiConfig& config, 
            SumiWindow &sumiWindow, 
            ControllerType type
        );

        void move(
            float dt, 
            const SumiWindow::KeypressEvents &keypressEvents,
            const glm::tvec2<double> &mouseDelta,
            const glm::mat4 &view,
            Transform3DComponent& transform
        );

        float moveSensitivity{5.0f};
        float sprintSensitivity{2.5f * moveSensitivity};
        
        float keyboardLookSensitivity{0.65f};
        float mouseLookSensitivity{0.05f};
        float cursorHidden{true};
        bool toggleShowCursor{true}; // hold to show cursor by default.

        float dtNoUpdateThreshold{0.2f}; // TODO: Match this with the Max frame time in sumire.cpp

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
            const glm::tvec2<double> &mouseDelta,
            const glm::mat4 &view,
            Transform3DComponent& transform
        );

        ControllerType type{WALK};
        SumiConfig& sumiConfig;
        SumiWindow& sumiWindow;
    };
}