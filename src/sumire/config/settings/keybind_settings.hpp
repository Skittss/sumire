#pragma once 

#include <sumire/core/windowing/sumi_key_input.hpp>

namespace sumire {

    struct DebugCameraKeybinds {
        int MOVE_LEFT     = SUMI_KEY_A;
        int MOVE_RIGHT    = SUMI_KEY_D;
        int MOVE_FORWARD  = SUMI_KEY_W;
        int MOVE_BACKWARD = SUMI_KEY_S;
        int MOVE_UP       = SUMI_KEY_SPACE;
        int MOVE_DOWN     = SUMI_KEY_LEFT_CONTROL;
        int LOOK_LEFT     = SUMI_KEY_LEFT;
        int LOOK_RIGHT    = SUMI_KEY_RIGHT;
        int LOOK_UP       = SUMI_KEY_UP;
        int LOOK_DOWN     = SUMI_KEY_DOWN;
        int SPRINT        = SUMI_KEY_LEFT_SHIFT;
        int TOGGLE_CURSOR = SUMI_KEY_LEFT_ALT;
    };

    struct KeybindSettings {
        DebugCameraKeybinds debugCamera{};
    };

}