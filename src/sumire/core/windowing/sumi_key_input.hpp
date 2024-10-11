#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Wrapper around key constants so that not explicitly reliant on choice of windowing API :)

namespace sumire {

    typedef int SumiKey;
    typedef int SumiKeyState;

    inline constexpr SumiKeyState SUMI_KEYSTATE_PRESS   = GLFW_PRESS;
    inline constexpr SumiKeyState SUMI_KEYSTATE_RELEASE = GLFW_RELEASE;
    inline constexpr SumiKeyState SUMI_KEYSTATE_REPEAT  = GLFW_REPEAT;
    
    inline constexpr SumiKey SUMI_KEY_UNKNOWN          = GLFW_KEY_UNKNOWN;

    // Printable keys
    inline constexpr SumiKey SUMI_KEY_SPACE            = GLFW_KEY_SPACE;
    inline constexpr SumiKey SUMI_KEY_APOSTROPHE       = GLFW_KEY_APOSTROPHE;
    inline constexpr SumiKey SUMI_KEY_COMMA            = GLFW_KEY_COMMA;
    inline constexpr SumiKey SUMI_KEY_MINUS            = GLFW_KEY_MINUS;
    inline constexpr SumiKey SUMI_KEY_PERIOD           = GLFW_KEY_PERIOD;
    inline constexpr SumiKey SUMI_KEY_SLASH            = GLFW_KEY_SLASH;
    inline constexpr SumiKey SUMI_KEY_0                = GLFW_KEY_0;
    inline constexpr SumiKey SUMI_KEY_1                = GLFW_KEY_1;
    inline constexpr SumiKey SUMI_KEY_2                = GLFW_KEY_2;
    inline constexpr SumiKey SUMI_KEY_3                = GLFW_KEY_3;
    inline constexpr SumiKey SUMI_KEY_4                = GLFW_KEY_4;
    inline constexpr SumiKey SUMI_KEY_5                = GLFW_KEY_5;
    inline constexpr SumiKey SUMI_KEY_6                = GLFW_KEY_6;
    inline constexpr SumiKey SUMI_KEY_7                = GLFW_KEY_7;
    inline constexpr SumiKey SUMI_KEY_8                = GLFW_KEY_8;
    inline constexpr SumiKey SUMI_KEY_9                = GLFW_KEY_9;
    inline constexpr SumiKey SUMI_KEY_SEMICOLON        = GLFW_KEY_SEMICOLON;
    inline constexpr SumiKey SUMI_KEY_EQUAL            = GLFW_KEY_EQUAL;
    inline constexpr SumiKey SUMI_KEY_A                = GLFW_KEY_A;
    inline constexpr SumiKey SUMI_KEY_B                = GLFW_KEY_B;
    inline constexpr SumiKey SUMI_KEY_C                = GLFW_KEY_C;
    inline constexpr SumiKey SUMI_KEY_D                = GLFW_KEY_D;
    inline constexpr SumiKey SUMI_KEY_E                = GLFW_KEY_E;
    inline constexpr SumiKey SUMI_KEY_F                = GLFW_KEY_F;
    inline constexpr SumiKey SUMI_KEY_G                = GLFW_KEY_G;
    inline constexpr SumiKey SUMI_KEY_H                = GLFW_KEY_H;
    inline constexpr SumiKey SUMI_KEY_I                = GLFW_KEY_I;
    inline constexpr SumiKey SUMI_KEY_J                = GLFW_KEY_J;
    inline constexpr SumiKey SUMI_KEY_K                = GLFW_KEY_K;
    inline constexpr SumiKey SUMI_KEY_L                = GLFW_KEY_L;
    inline constexpr SumiKey SUMI_KEY_M                = GLFW_KEY_M;
    inline constexpr SumiKey SUMI_KEY_N                = GLFW_KEY_N;
    inline constexpr SumiKey SUMI_KEY_O                = GLFW_KEY_O;
    inline constexpr SumiKey SUMI_KEY_P                = GLFW_KEY_P;
    inline constexpr SumiKey SUMI_KEY_Q                = GLFW_KEY_Q;
    inline constexpr SumiKey SUMI_KEY_R                = GLFW_KEY_R;
    inline constexpr SumiKey SUMI_KEY_S                = GLFW_KEY_S;
    inline constexpr SumiKey SUMI_KEY_T                = GLFW_KEY_T;
    inline constexpr SumiKey SUMI_KEY_U                = GLFW_KEY_U;
    inline constexpr SumiKey SUMI_KEY_V                = GLFW_KEY_V;
    inline constexpr SumiKey SUMI_KEY_W                = GLFW_KEY_W;
    inline constexpr SumiKey SUMI_KEY_X                = GLFW_KEY_X;
    inline constexpr SumiKey SUMI_KEY_Y                = GLFW_KEY_Y;
    inline constexpr SumiKey SUMI_KEY_Z                = GLFW_KEY_Z;
    inline constexpr SumiKey SUMI_KEY_LEFT_BRACKET     = GLFW_KEY_LEFT_BRACKET;
    inline constexpr SumiKey SUMI_KEY_BACKSLASH        = GLFW_KEY_BACKSLASH;
    inline constexpr SumiKey SUMI_KEY_RIGHT_BRACKET    = GLFW_KEY_RIGHT_BRACKET;
    inline constexpr SumiKey SUMI_KEY_GRAVE_ACCENT     = GLFW_KEY_GRAVE_ACCENT;
    inline constexpr SumiKey SUMI_KEY_WORLD_1          = GLFW_KEY_WORLD_1;
    inline constexpr SumiKey SUMI_KEY_WORLD_2          = GLFW_KEY_WORLD_2;

    // Function Keys
    inline constexpr SumiKey SUMI_KEY_ESCAPE           = GLFW_KEY_ESCAPE;
    inline constexpr SumiKey SUMI_KEY_ENTER            = GLFW_KEY_ENTER;
    inline constexpr SumiKey SUMI_KEY_TAB              = GLFW_KEY_TAB;
    inline constexpr SumiKey SUMI_KEY_BACKSPACE        = GLFW_KEY_BACKSPACE;
    inline constexpr SumiKey SUMI_KEY_INSERT           = GLFW_KEY_INSERT;
    inline constexpr SumiKey SUMI_KEY_DELETE           = GLFW_KEY_DELETE;
    inline constexpr SumiKey SUMI_KEY_RIGHT            = GLFW_KEY_RIGHT;
    inline constexpr SumiKey SUMI_KEY_LEFT             = GLFW_KEY_LEFT;
    inline constexpr SumiKey SUMI_KEY_DOWN             = GLFW_KEY_DOWN;
    inline constexpr SumiKey SUMI_KEY_UP               = GLFW_KEY_UP;
    inline constexpr SumiKey SUMI_KEY_PAGE_UP          = GLFW_KEY_PAGE_UP;
    inline constexpr SumiKey SUMI_KEY_PAGE_DOWN        = GLFW_KEY_PAGE_DOWN;
    inline constexpr SumiKey SUMI_KEY_HOME             = GLFW_KEY_HOME;
    inline constexpr SumiKey SUMI_KEY_END              = GLFW_KEY_END;
    inline constexpr SumiKey SUMI_KEY_CAPS_LOCK        = GLFW_KEY_CAPS_LOCK;
    inline constexpr SumiKey SUMI_KEY_SCROLL_LOCK      = GLFW_KEY_SCROLL_LOCK;
    inline constexpr SumiKey SUMI_KEY_NUM_LOCK         = GLFW_KEY_NUM_LOCK;
    inline constexpr SumiKey SUMI_KEY_PRINT_SCREEN     = GLFW_KEY_PRINT_SCREEN;
    inline constexpr SumiKey SUMI_KEY_PAUSE            = GLFW_KEY_PAUSE;
    inline constexpr SumiKey SUMI_KEY_F1               = GLFW_KEY_F1;
    inline constexpr SumiKey SUMI_KEY_F2               = GLFW_KEY_F2;
    inline constexpr SumiKey SUMI_KEY_F3               = GLFW_KEY_F3;
    inline constexpr SumiKey SUMI_KEY_F4               = GLFW_KEY_F4;
    inline constexpr SumiKey SUMI_KEY_F5               = GLFW_KEY_F5;
    inline constexpr SumiKey SUMI_KEY_F6               = GLFW_KEY_F6;
    inline constexpr SumiKey SUMI_KEY_F7               = GLFW_KEY_F7;
    inline constexpr SumiKey SUMI_KEY_F8               = GLFW_KEY_F8;
    inline constexpr SumiKey SUMI_KEY_F9               = GLFW_KEY_F9;
    inline constexpr SumiKey SUMI_KEY_F10              = GLFW_KEY_F10;
    inline constexpr SumiKey SUMI_KEY_F11              = GLFW_KEY_F11;
    inline constexpr SumiKey SUMI_KEY_F12              = GLFW_KEY_F12;
    inline constexpr SumiKey SUMI_KEY_F13              = GLFW_KEY_F13;
    inline constexpr SumiKey SUMI_KEY_F14              = GLFW_KEY_F14;
    inline constexpr SumiKey SUMI_KEY_F15              = GLFW_KEY_F15;
    inline constexpr SumiKey SUMI_KEY_F16              = GLFW_KEY_F16;
    inline constexpr SumiKey SUMI_KEY_F17              = GLFW_KEY_F17;
    inline constexpr SumiKey SUMI_KEY_F18              = GLFW_KEY_F18;
    inline constexpr SumiKey SUMI_KEY_F19              = GLFW_KEY_F19;
    inline constexpr SumiKey SUMI_KEY_F20              = GLFW_KEY_F20;
    inline constexpr SumiKey SUMI_KEY_F21              = GLFW_KEY_F21;
    inline constexpr SumiKey SUMI_KEY_F22              = GLFW_KEY_F22;
    inline constexpr SumiKey SUMI_KEY_F23              = GLFW_KEY_F23;
    inline constexpr SumiKey SUMI_KEY_F24              = GLFW_KEY_F24;
    inline constexpr SumiKey SUMI_KEY_F25              = GLFW_KEY_F25;
    inline constexpr SumiKey SUMI_KEY_KP_0             = GLFW_KEY_KP_0;
    inline constexpr SumiKey SUMI_KEY_KP_1             = GLFW_KEY_KP_1;
    inline constexpr SumiKey SUMI_KEY_KP_2             = GLFW_KEY_KP_2;
    inline constexpr SumiKey SUMI_KEY_KP_3             = GLFW_KEY_KP_3;
    inline constexpr SumiKey SUMI_KEY_KP_4             = GLFW_KEY_KP_4;
    inline constexpr SumiKey SUMI_KEY_KP_5             = GLFW_KEY_KP_5;
    inline constexpr SumiKey SUMI_KEY_KP_6             = GLFW_KEY_KP_6;
    inline constexpr SumiKey SUMI_KEY_KP_7             = GLFW_KEY_KP_7;
    inline constexpr SumiKey SUMI_KEY_KP_8             = GLFW_KEY_KP_8;
    inline constexpr SumiKey SUMI_KEY_KP_9             = GLFW_KEY_KP_9;
    inline constexpr SumiKey SUMI_KEY_KP_DECIMAL       = GLFW_KEY_KP_DECIMAL;
    inline constexpr SumiKey SUMI_KEY_KP_DIVIDE        = GLFW_KEY_KP_DIVIDE;
    inline constexpr SumiKey SUMI_KEY_KP_MULTIPLY      = GLFW_KEY_KP_MULTIPLY;
    inline constexpr SumiKey SUMI_KEY_KP_SUBTRACT      = GLFW_KEY_KP_SUBTRACT;
    inline constexpr SumiKey SUMI_KEY_KP_ADD           = GLFW_KEY_KP_ADD;
    inline constexpr SumiKey SUMI_KEY_KP_ENTER         = GLFW_KEY_KP_ENTER;
    inline constexpr SumiKey SUMI_KEY_KP_EQUAL         = GLFW_KEY_KP_EQUAL;
    inline constexpr SumiKey SUMI_KEY_LEFT_SHIFT       = GLFW_KEY_LEFT_SHIFT;
    inline constexpr SumiKey SUMI_KEY_LEFT_CONTROL     = GLFW_KEY_LEFT_CONTROL;
    inline constexpr SumiKey SUMI_KEY_LEFT_ALT         = GLFW_KEY_LEFT_ALT;
    inline constexpr SumiKey SUMI_KEY_LEFT_SUPER       = GLFW_KEY_LEFT_SUPER;
    inline constexpr SumiKey SUMI_KEY_RIGHT_SHIFT      = GLFW_KEY_RIGHT_SHIFT;
    inline constexpr SumiKey SUMI_KEY_RIGHT_CONTROL    = GLFW_KEY_RIGHT_CONTROL;
    inline constexpr SumiKey SUMI_KEY_RIGHT_ALT        = GLFW_KEY_RIGHT_ALT;
    inline constexpr SumiKey SUMI_KEY_RIGHT_SUPER      = GLFW_KEY_RIGHT_SUPER;
    inline constexpr SumiKey SUMI_KEY_MENU             = GLFW_KEY_MENU;
    
    // Key Modifiers
    inline constexpr SumiKey SUMI_MOD_SHIFT            = GLFW_MOD_SHIFT;
    inline constexpr SumiKey SUMI_MOD_CONTROL          = GLFW_MOD_CONTROL;
    inline constexpr SumiKey SUMI_MOD_ALT              = GLFW_MOD_ALT;
    inline constexpr SumiKey SUMI_MOD_SUPER            = GLFW_MOD_SUPER;
    
    // Mouse Buttons
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_1       = GLFW_MOUSE_BUTTON_1;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_2       = GLFW_MOUSE_BUTTON_2;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_3       = GLFW_MOUSE_BUTTON_3;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_4       = GLFW_MOUSE_BUTTON_4;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_5       = GLFW_MOUSE_BUTTON_5;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_6       = GLFW_MOUSE_BUTTON_6;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_7       = GLFW_MOUSE_BUTTON_7;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_8       = GLFW_MOUSE_BUTTON_8;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_LEFT    = GLFW_MOUSE_BUTTON_LEFT;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_RIGHT   = GLFW_MOUSE_BUTTON_RIGHT;
    inline constexpr SumiKey SUMI_MOUSE_BUTTON_MIDDLE  = GLFW_MOUSE_BUTTON_MIDDLE;

    // Joystick Buttons
    inline constexpr SumiKey SUMI_JOYSTICK_1           = GLFW_JOYSTICK_1;
    inline constexpr SumiKey SUMI_JOYSTICK_2           = GLFW_JOYSTICK_2;
    inline constexpr SumiKey SUMI_JOYSTICK_3           = GLFW_JOYSTICK_3;
    inline constexpr SumiKey SUMI_JOYSTICK_4           = GLFW_JOYSTICK_4;
    inline constexpr SumiKey SUMI_JOYSTICK_5           = GLFW_JOYSTICK_5;
    inline constexpr SumiKey SUMI_JOYSTICK_6           = GLFW_JOYSTICK_6;
    inline constexpr SumiKey SUMI_JOYSTICK_7           = GLFW_JOYSTICK_7;
    inline constexpr SumiKey SUMI_JOYSTICK_8           = GLFW_JOYSTICK_8;
    inline constexpr SumiKey SUMI_JOYSTICK_9           = GLFW_JOYSTICK_9;
    inline constexpr SumiKey SUMI_JOYSTICK_10          = GLFW_JOYSTICK_10;
    inline constexpr SumiKey SUMI_JOYSTICK_11          = GLFW_JOYSTICK_11;
    inline constexpr SumiKey SUMI_JOYSTICK_12          = GLFW_JOYSTICK_12;
    inline constexpr SumiKey SUMI_JOYSTICK_13          = GLFW_JOYSTICK_13;
    inline constexpr SumiKey SUMI_JOYSTICK_14          = GLFW_JOYSTICK_14;
    inline constexpr SumiKey SUMI_JOYSTICK_15          = GLFW_JOYSTICK_15;
    inline constexpr SumiKey SUMI_JOYSTICK_16          = GLFW_JOYSTICK_16;

}