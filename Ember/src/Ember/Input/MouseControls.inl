// The single source of truth for every mouse control Ember knows about.
//
// Included repeatedly with a different EB_MOUSE_CONTROL definition each time, so the enum and the
// name tables read from this one list and a new control is added in exactly one place.
// Deliberately no #pragma once, and deliberately no include guard.
//
// Button values match GLFW's GLFW_MOUSE_BUTTON_* numbering, so the first three line up with
// MouseButton. The wheel entries are synthetic and start past the button block to leave room.

EB_MOUSE_CONTROL(Left,      0)    /* GLFW_MOUSE_BUTTON_1 */
EB_MOUSE_CONTROL(Right,     1)    /* GLFW_MOUSE_BUTTON_2 */
EB_MOUSE_CONTROL(Middle,    2)    /* GLFW_MOUSE_BUTTON_3 */
EB_MOUSE_CONTROL(Button4,   3)    /* often XButton1 / Back */
EB_MOUSE_CONTROL(Button5,   4)    /* often XButton2 / Forward */
EB_MOUSE_CONTROL(Button6,   5)
EB_MOUSE_CONTROL(Button7,   6)
EB_MOUSE_CONTROL(Button8,   7)
EB_MOUSE_CONTROL(Button9,   8)
EB_MOUSE_CONTROL(Button10,  9)
EB_MOUSE_CONTROL(Button11,  10)
EB_MOUSE_CONTROL(Button12,  11)
EB_MOUSE_CONTROL(Button13,  12)
EB_MOUSE_CONTROL(Button14,  13)
EB_MOUSE_CONTROL(Button15,  14)

/* Wheel - synthetic controls, not GLFW buttons */
EB_MOUSE_CONTROL(WheelUp,   16)
EB_MOUSE_CONTROL(WheelDown, 17)
