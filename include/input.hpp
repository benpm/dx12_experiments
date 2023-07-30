#pragma once

#include <gainput/gainput.h>
#include <common.hpp>

enum class Key : UINT {
    A = (UINT)('A'),
    B = (UINT)('B'),
    C = (UINT)('C'),
    D = (UINT)('D'),
    E = (UINT)('E'),
    F = (UINT)('F'),
    G = (UINT)('G'),
    H = (UINT)('H'),
    I = (UINT)('I'),
    J = (UINT)('J'),
    K = (UINT)('K'),
    L = (UINT)('L'),
    M = (UINT)('M'),
    N = (UINT)('N'),
    O = (UINT)('O'),
    P = (UINT)('P'),
    Q = (UINT)('Q'),
    R = (UINT)('R'),
    S = (UINT)('S'),
    T = (UINT)('T'),
    U = (UINT)('U'),
    V = (UINT)('V'),
    W = (UINT)('W'),
    X = (UINT)('X'),
    Y = (UINT)('Y'),
    Z = (UINT)('Z'),
    Num0 = (UINT)('0'),
    Num1 = (UINT)('1'),
    Num2 = (UINT)('2'),
    Num3 = (UINT)('3'),
    Num4 = (UINT)('4'),
    Num5 = (UINT)('5'),
    Num6 = (UINT)('6'),
    Num7 = (UINT)('7'),
    Num8 = (UINT)('8'),
    Num9 = (UINT)('9'),
    Escape = VK_ESCAPE,
    LControl = VK_LCONTROL,
    LShift = VK_LSHIFT,
    LAlt = VK_LMENU,
    LSystem = VK_LWIN,
    RControl = VK_RCONTROL,
    RShift = VK_RSHIFT,
    RAlt = VK_RMENU,
    RSystem = VK_RWIN,
    Menu = VK_APPS,
    LBracket = VK_OEM_4,
    RBracket = VK_OEM_6,
    SemiColon = VK_OEM_1,
    Comma = VK_OEM_COMMA,
    Period = VK_OEM_PERIOD,
    Quote = VK_OEM_7,
    Slash = VK_OEM_2,
    BackSlash = VK_OEM_5,
    Tilde = VK_OEM_3,
    Equal = VK_OEM_PLUS,
    Dash = VK_OEM_MINUS,
    Space = VK_SPACE,
    F1 = VK_F1,
    F2 = VK_F2,
    F3 = VK_F3,
    F4 = VK_F4,
    F5 = VK_F5,
    F6 = VK_F6,
    F7 = VK_F7,
    F8 = VK_F8,
    F9 = VK_F9,
    F10 = VK_F10,
    F11 = VK_F11,
    F12 = VK_F12,
    F13 = VK_F13,
    F14 = VK_F14,
    F15 = VK_F15,
    Pause = VK_PAUSE,
    Insert = VK_INSERT,
    Home = VK_HOME,
    PageUp = VK_PRIOR,
    Delete = VK_DELETE,
    End = VK_END,
    PageDown = VK_NEXT,
    Right = VK_RIGHT,
    Left = VK_LEFT,
    Down = VK_DOWN,
    Up = VK_UP,
    Numpad0 = VK_NUMPAD0,
    Numpad1 = VK_NUMPAD1,
    Numpad2 = VK_NUMPAD2,
    Numpad3 = VK_NUMPAD3,
    Numpad4 = VK_NUMPAD4,
    Numpad5 = VK_NUMPAD5,
    Numpad6 = VK_NUMPAD6,
    Numpad7 = VK_NUMPAD7,
    Numpad8 = VK_NUMPAD8,
    Numpad9 = VK_NUMPAD9,
    NumpadEnter = VK_RETURN,
    NumpadAdd = VK_ADD,
    NumpadSubtract = VK_SUBTRACT,
    NumpadMultiply = VK_MULTIPLY,
    NumpadDivide = VK_DIVIDE,
    Backspace = VK_BACK,
    Tab = VK_TAB,
    CapsLock = VK_CAPITAL,
    Enter = VK_RETURN,
    Shift = VK_SHIFT,
    Control = VK_CONTROL,
    Alt = VK_MENU,
    Plus = VK_OEM_PLUS,
    Minus = VK_OEM_MINUS,
    Period2 = VK_DECIMAL,
    NumLock = VK_NUMLOCK,
    ScrollLock = VK_SCROLL
};
enum class MouseButton { Left, Right, Middle, XButton1, XButton2 };
namespace Button {
enum : gainput::UserButtonId {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Interact,
    AxisX,
    AxisY,
    AxisDeltaX,
    AxisDeltaY,
    Exit,
};
}

struct EventPaint {};
struct EventKeyDown {
    Key key;
};
struct EventKeyUp {
    Key key;
};
struct EventMouseMove {
    int x, y;
};
struct EventMouseWheel {
    int delta;
};
struct EventMouseButtonDown {
    MouseButton button;
};
struct EventMouseButtonUp {
    MouseButton button;
};
struct EventResize {
    uint32_t width, height;
};

extern gainput::InputManager inputManger;