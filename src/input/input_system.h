#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct GLFWwindow GLFWwindow;

typedef enum
{
    InputMod_Shift = (1 << 0),
    InputMod_Ctrl = (1 << 1),
    InputMod_Alt = (1 << 2),
    InputMod_Super = (1 << 3),

    InputMod_Mask = (1 << 4) - 1
} InputMod;

typedef enum
{
    KeyCode_Invalid = 0,

    KeyCode_Space = 32,
    KeyCode_Apostrophe = 39,    /* ' */
    KeyCode_Comma = 44,         /* , */
    KeyCode_Minus = 45,         /* - */
    KeyCode_Period = 46,        /* . */
    KeyCode_Slash = 47,         /* / */

    KeyCode_0 = 48,
    KeyCode_1 = 49,
    KeyCode_2 = 50,
    KeyCode_3 = 51,
    KeyCode_4 = 52,
    KeyCode_5 = 53,
    KeyCode_6 = 54,
    KeyCode_7 = 55,
    KeyCode_8 = 56,
    KeyCode_9 = 57,

    KeyCode_Semicolon = 59,     /* ; */
    KeyCode_Equal = 61,         /* = */

    KeyCode_A = 65,
    KeyCode_B = 66,
    KeyCode_C = 67,
    KeyCode_D = 68,
    KeyCode_E = 69,
    KeyCode_F = 70,
    KeyCode_G = 71,
    KeyCode_H = 72,
    KeyCode_I = 73,
    KeyCode_J = 74,
    KeyCode_K = 75,
    KeyCode_L = 76,
    KeyCode_M = 77,
    KeyCode_N = 78,
    KeyCode_O = 79,
    KeyCode_P = 80,
    KeyCode_Q = 81,
    KeyCode_R = 82,
    KeyCode_S = 83,
    KeyCode_T = 84,
    KeyCode_U = 85,
    KeyCode_V = 86,
    KeyCode_W = 87,
    KeyCode_X = 88,
    KeyCode_Y = 89,
    KeyCode_Z = 90,

    KeyCode_LeftBracket = 91,       /* [ */
    KeyCode_Backslash = 92,         /* \ */
    KeyCode_RightBracket = 93,      /* ] */
    KeyCode_GraveAccent = 96,       /* ` */

    KeyCode_World1 = 161, /* non-US #1 */
    KeyCode_World2 = 162, /* non-US #2 */

    KeyCode_Escape = 256,
    KeyCode_Enter = 257,
    KeyCode_Tab = 258,
    KeyCode_Backspace = 259,
    KeyCode_Insert = 260,
    KeyCode_Delete = 261,
    KeyCode_Right = 262,
    KeyCode_Left = 263,
    KeyCode_Down = 264,
    KeyCode_Up = 265,
    KeyCode_PageUp = 266,
    KeyCode_PageDown = 267,
    KeyCode_Home = 268,
    KeyCode_End = 269,
    KeyCode_CapsLock = 280,
    KeyCode_ScrollLock = 281,
    KeyCode_NumLock = 282,
    KeyCode_PrintScreen = 283,
    KeyCode_Pause = 284,
    KeyCode_F1 = 290,
    KeyCode_F2 = 291,
    KeyCode_F3 = 292,
    KeyCode_F4 = 293,
    KeyCode_F5 = 294,
    KeyCode_F6 = 295,
    KeyCode_F7 = 296,
    KeyCode_F8 = 297,
    KeyCode_F9 = 298,
    KeyCode_F10 = 299,
    KeyCode_F11 = 300,
    KeyCode_F12 = 301,
    KeyCode_KP_0 = 320,
    KeyCode_KP_1 = 321,
    KeyCode_KP_2 = 322,
    KeyCode_KP_3 = 323,
    KeyCode_KP_4 = 324,
    KeyCode_KP_5 = 325,
    KeyCode_KP_6 = 326,
    KeyCode_KP_7 = 327,
    KeyCode_KP_8 = 328,
    KeyCode_KP_9 = 329,
    KeyCode_KP_Decimal = 330,
    KeyCode_KP_Divide = 331,
    KeyCode_KP_Multiply = 332,
    KeyCode_KP_Subtract = 333,
    KeyCode_KP_Add = 334,
    KeyCode_KP_Enter = 335,
    KeyCode_KP_Equal = 336,
    KeyCode_LeftShift = 340,
    KeyCode_LeftControl = 341,
    KeyCode_LeftAlt = 342,
    KeyCode_LeftSuper = 343,
    KeyCode_RightShift = 344,
    KeyCode_RightControl = 345,
    KeyCode_RightAlt = 346,
    KeyCode_RightSuper = 347,
    KeyCode_Menu = 348,

    KeyCode_COUNT
} Pim_KeyCode;

typedef enum
{
    MouseButton_Left = 0,
    MouseButton_Right,
    MouseButton_Middle,

    MouseButton_COUNT
} MouseButton;

typedef enum
{
    MouseAxis_X = 0,
    MouseAxis_Y,
    MouseAxis_ScrollX,
    MouseAxis_ScrollY,

    MouseAxis_COUNT
} MouseAxis;

void InputSys_Init(void);
void InputSys_Update(void);
void InputSys_Shutdown(void);

void Input_RegWindow(GLFWwindow* window);

GLFWwindow* Input_GetFocus(void);
void Input_SetFocus(GLFWwindow* window);

bool Input_IsCursorCaptured(GLFWwindow* window);
void Input_CaptureCursor(GLFWwindow* window, bool capture);

bool Input_GetKey(Pim_KeyCode key);
bool Input_GetButton(MouseButton button);

bool Input_IsKeyDown(Pim_KeyCode key);
bool Input_IsButtonDown(MouseButton button);

bool Input_IsKeyUp(Pim_KeyCode key);
bool Input_IsButtonUp(MouseButton button);

float Input_GetAxis(MouseAxis axis);
float Input_GetDeltaAxis(MouseAxis axis);

PIM_C_END
