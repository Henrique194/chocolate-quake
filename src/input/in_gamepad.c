/*
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) 2025 Henrique Barateli <henriquejb194@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
// in_gamepad.c -- gamepad code


#include "in_gamepad.h"
#include "console.h"
#include "keys.h"
#include "sys.h"
#include <SDL.h>


static cvar_t joy_deadzone_look = {"joy_deadzone_look", "0.175", true};
static cvar_t joy_deadzone_move = {"joy_deadzone_move", "0.175", true};
static cvar_t joy_outer_threshold_look = {"joy_outer_threshold_look", "0.02", true};
static cvar_t joy_outer_threshold_move = {"joy_outer_threshold_move", "0.02", true};
static cvar_t joy_deadzone_trigger = {"joy_deadzone_trigger", "0.2", true};
static cvar_t joy_sensitivity_yaw = {"joy_sensitivity_yaw", "240", true};
static cvar_t joy_sensitivity_pitch = {"joy_sensitivity_pitch", "130", true};
static cvar_t joy_invert = {"joy_invert", "0", true};
static cvar_t joy_exponent = {"joy_exponent", "2", true};
static cvar_t joy_exponent_move = {"joy_exponent_move", "2", true};
static cvar_t joy_swapmovelook = {"joy_swapmovelook", "0", true};
static cvar_t joy_enable = {"joy_enable", "1", true};

typedef struct {
    qboolean was_pressed; // The previous state of the button.
    qboolean pressed;     // The current state of the button.
    int key;              // The key code associated with this button.
    double timer;         // Timer to manage repeated button presses.
} button_t;

typedef struct {
    float previous_value;
    float value;
} axis_t;

typedef struct {
    float x; // Horizontal axis value
    float y; // Vertical axis value
} analog_stick_t;

typedef enum {
    EMULATE_NONE,
    EMULATE_ANALOG_STICK,
    EMULATE_TRIGGER,
} emulate_axis_t;

typedef struct {
    const emulate_axis_t type; // Type of emulation.
    const float* threshold;    // Threshold value to trigger button press.
    button_t* pos_button;      // Button for positive direction.
    button_t* neg_button;      // Button for negative direction.
} axis_button_mapping_t;

static button_t buttons[SDL_CONTROLLER_BUTTON_MAX] = {
    [SDL_CONTROLLER_BUTTON_A] = {.key = K_ABUTTON},
    [SDL_CONTROLLER_BUTTON_B] = {.key = K_BBUTTON},
    [SDL_CONTROLLER_BUTTON_X] = {.key = K_XBUTTON},
    [SDL_CONTROLLER_BUTTON_Y] = {.key = K_YBUTTON},
    [SDL_CONTROLLER_BUTTON_BACK] = {.key = K_TAB},
    [SDL_CONTROLLER_BUTTON_START] = {.key = K_ESCAPE},
    [SDL_CONTROLLER_BUTTON_LEFTSTICK] = {.key = K_LTHUMB},
    [SDL_CONTROLLER_BUTTON_RIGHTSTICK] = {.key = K_RTHUMB},
    [SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = {.key = K_LSHOULDER},
    [SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = {.key = K_RSHOULDER},
    [SDL_CONTROLLER_BUTTON_DPAD_UP] = {.key = K_UPARROW},
    [SDL_CONTROLLER_BUTTON_DPAD_DOWN] = {.key = K_DOWNARROW},
    [SDL_CONTROLLER_BUTTON_DPAD_LEFT] = {.key = K_LEFTARROW},
    [SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = {.key = K_RIGHTARROW},
};
static button_t left_trigger_button = {.key = K_LTRIGGER};
static button_t right_trigger_button = {.key = K_RTRIGGER};

static axis_t axes[SDL_CONTROLLER_AXIS_MAX] = {0};

static SDL_GameController* gamepad = NULL;
static SDL_JoystickID gamepad_id = -1;


/*
================================================================================

BUTTON EMULATION FOR AXES

================================================================================
*/

static const float analog_stick_threshold = 0.9f;

static axis_button_mapping_t axes_mapping[SDL_CONTROLLER_AXIS_MAX] = {
    [SDL_CONTROLLER_AXIS_LEFTX] = {
        .type = EMULATE_ANALOG_STICK,
        .threshold = &analog_stick_threshold,
        .pos_button = &buttons[SDL_CONTROLLER_BUTTON_DPAD_RIGHT],
        .neg_button = &buttons[SDL_CONTROLLER_BUTTON_DPAD_LEFT],
    },
    [SDL_CONTROLLER_AXIS_LEFTY] = {
        .type = EMULATE_ANALOG_STICK,
        .threshold = &analog_stick_threshold,
        .pos_button = &buttons[SDL_CONTROLLER_BUTTON_DPAD_DOWN],
        .neg_button = &buttons[SDL_CONTROLLER_BUTTON_DPAD_UP],
    },
    [SDL_CONTROLLER_AXIS_TRIGGERLEFT] = {
        .type = EMULATE_TRIGGER,
        .threshold = &joy_deadzone_trigger.value,
        .pos_button = &left_trigger_button,
    },
    [SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = {
        .type = EMULATE_TRIGGER,
        .threshold = &joy_deadzone_trigger.value,
        .pos_button = &right_trigger_button,
    },
};

static void IN_SendButtonEvent(button_t* button);

static void IN_EmulateButtonPress(const axis_t* axis,
                                  const axis_button_mapping_t* mapping) {
    if (mapping->pos_button) {
        button_t* button = mapping->pos_button;
        const float threshold = *mapping->threshold;
        button->was_pressed = (axis->previous_value > threshold);
        button->pressed = (axis->value > threshold);
        IN_SendButtonEvent(button);
    }
    if (mapping->neg_button) {
        button_t* button = mapping->neg_button;
        const float threshold = *mapping->threshold;
        button->was_pressed = (-axis->previous_value > threshold);
        button->pressed = (-axis->value > threshold);
        IN_SendButtonEvent(button);
    }
}

static void IN_EmulateButtonForAxis(const SDL_GameControllerAxis axis_type) {
    const axis_t* axis = &axes[axis_type];
    const axis_button_mapping_t* mapping = &axes_mapping[axis_type];
    switch (mapping->type) {
        case EMULATE_ANALOG_STICK:
            if (key_dest != key_game) {
                IN_EmulateButtonPress(axis, mapping);
            }
            break;
        case EMULATE_TRIGGER:
            IN_EmulateButtonPress(axis, mapping);
            break;
        default:
            break;
    }
}


//==============================================================================


/*
================================================================================

GAMEPAD EVENT

================================================================================
*/

static void IN_SendButtonEvent(button_t* button) {
    // Using `realtime` could cause issues with key repeats
    // due to non-monotonic behavior.
    const double now = Sys_FloatTime();
    if (!button->was_pressed) {
        if (button->pressed) {
            button->timer = now + 0.5;
            Key_Event(button->key, true);
        }
        return;
    }
    if (button->pressed) {
        if (now >= button->timer) {
            button->timer = now + 0.1;
            Key_Event(button->key, true);
        }
        return;
    }
    button->timer = 0;
    Key_Event(button->key, false);
}

static void IN_AxisEvent(const SDL_ControllerAxisEvent* event) {
    if (event->which != gamepad_id) {
        return;
    }
    axis_t* axis = &axes[event->axis];
    axis->previous_value = axis->value;
    axis->value = (float) event->value / 32768.0f;
    IN_EmulateButtonForAxis(event->axis);
}

static void IN_ButtonEvent(const SDL_ControllerButtonEvent* event) {
    if (event->which != gamepad_id) {
        return;
    }
    button_t* button = &buttons[event->button];
    button->was_pressed = button->pressed;
    button->pressed = (event->state == SDL_PRESSED);
    IN_SendButtonEvent(button);
}

static void IN_RemovedControllerEvent(const SDL_ControllerDeviceEvent* event) {
    if (event->which != gamepad_id) {
        return;
    }
    SDL_GameControllerClose(gamepad);
    gamepad = NULL;
    gamepad_id = -1;
}

static void IN_AddedControllerEvent(const SDL_ControllerDeviceEvent* event) {
    if (gamepad) {
        // Already have one gamepad connected.
        return;
    }
    gamepad = SDL_GameControllerOpen(event->which);
    if (!gamepad) {
        Con_Printf("\ncould not open joystick: (%s)\n\n", SDL_GetError());
        return;
    }
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);
    gamepad_id = SDL_JoystickInstanceID(joystick);
    Con_Printf("\njoystick detected\n\n");
}

void IN_GamepadEvent(const SDL_Event* event) {
    if (joy_enable.value == 0) {
        return;
    }
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED:
            IN_AddedControllerEvent(&event->cdevice);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            IN_RemovedControllerEvent(&event->cdevice);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            IN_ButtonEvent(&event->cbutton);
            break;
        case SDL_CONTROLLERAXISMOTION:
            IN_AxisEvent(&event->caxis);
            break;
        default:
            break;
    }
}

//==============================================================================


/*
================================================================================

GAMEPAD MOVE

================================================================================
*/

static vec_t IN_MoveMagnitude(const analog_stick_t* stick) {
    return sqrtf((stick->x * stick->x) + (stick->y * stick->y));
}

static void IN_ApplyEasing(analog_stick_t* stick, const float exponent) {
    const vec_t magnitude = IN_MoveMagnitude(stick);
    if (magnitude == 0) {
        return;
    }
    const vec_t eased_magnitude = powf(magnitude, exponent);
    const vec_t scale = eased_magnitude / magnitude;
    stick->x *= scale;
    stick->y *= scale;
}

static void IN_ApplyDeadzone(analog_stick_t* stick, const float deadzone,
                             const float outer_threshold) {
    const vec_t magnitude = IN_MoveMagnitude(stick);
    if (magnitude <= deadzone) {
        // Within deadzone, so no move.
        stick->x = 0;
        stick->y = 0;
        return;
    }
    // Rescale the magnitude so deadzone becomes 0 and 1-outer_threshold becomes 1.
    const vec_t range = (magnitude - deadzone);
    const vec_t full_range = (1.0f - deadzone - outer_threshold);
    vec_t new_magnitude = range / full_range;
    new_magnitude = SDL_min(new_magnitude, 1.0f);
    const vec_t scale = new_magnitude / magnitude;
    stick->x *= scale;
    stick->y *= scale;
}

static void IN_ReadAnalogSticks(analog_stick_t* left, analog_stick_t* right) {
    float deadzone = joy_deadzone_move.value;
    float outer_threshold = joy_outer_threshold_move.value;
    float exponent = joy_exponent_move.value;
    left->x = axes[SDL_CONTROLLER_AXIS_LEFTX].value;
    left->y = axes[SDL_CONTROLLER_AXIS_LEFTY].value;
    IN_ApplyDeadzone(left, deadzone, outer_threshold);
    IN_ApplyEasing(left, exponent);

    deadzone = joy_deadzone_look.value;
    outer_threshold = joy_outer_threshold_look.value;
    exponent = joy_exponent.value;
    right->x = axes[SDL_CONTROLLER_AXIS_RIGHTX].value;
    right->y = axes[SDL_CONTROLLER_AXIS_RIGHTY].value;
    IN_ApplyDeadzone(right, deadzone, outer_threshold);
    IN_ApplyEasing(right, exponent);

    if (joy_swapmovelook.value != 0) {
        const analog_stick_t temp = *left;
        *left = *right;
        *right = temp;
    }
}

static void IN_MoveCamera(const analog_stick_t* look) {
    const float time = (float) host_frametime;
    const float pitch_dir = (joy_invert.value != 0 ? -1.0f : 1.0f);

    float joy_sensitivity = joy_sensitivity_yaw.value;
    cl.viewangles[YAW] -= look->x * joy_sensitivity * time;

    joy_sensitivity = joy_sensitivity_pitch.value;
    cl.viewangles[PITCH] += look->y * joy_sensitivity * pitch_dir * time;
    cl.viewangles[PITCH] = SDL_clamp(cl.viewangles[PITCH], -70, 80);

    if (look->x != 0 || look->y != 0) {
        V_StopPitchDrift();
    }
}

static void IN_MovePlayer(usercmd_t* cmd, const analog_stick_t* move) {
    float speed = cl_forwardspeed.value;
    if (in_speed.state & 1) {
        speed *= cl_movespeedkey.value;
    }
    cmd->sidemove += speed * move->x;
    cmd->forwardmove -= speed * move->y;
}

void IN_JoyMove(usercmd_t* cmd) {
    if (joy_enable.value == 0 || !gamepad) {
        return;
    }
    if (cl.paused || key_dest != key_game) {
        return;
    }
    analog_stick_t left;
    analog_stick_t right;
    IN_ReadAnalogSticks(&left, &right);
    IN_MovePlayer(cmd, &left);
    IN_MoveCamera(&right);
}


//==============================================================================


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static void IN_RegisterCvars(void) {
    Cvar_RegisterVariable(&joy_sensitivity_yaw);
    Cvar_RegisterVariable(&joy_sensitivity_pitch);
    Cvar_RegisterVariable(&joy_deadzone_look);
    Cvar_RegisterVariable(&joy_deadzone_move);
    Cvar_RegisterVariable(&joy_outer_threshold_look);
    Cvar_RegisterVariable(&joy_outer_threshold_move);
    Cvar_RegisterVariable(&joy_deadzone_trigger);
    Cvar_RegisterVariable(&joy_invert);
    Cvar_RegisterVariable(&joy_exponent);
    Cvar_RegisterVariable(&joy_exponent_move);
    Cvar_RegisterVariable(&joy_swapmovelook);
    Cvar_RegisterVariable(&joy_enable);
}

void IN_InitGamepad(void) {
    IN_RegisterCvars();
    if (COM_CheckParm("-nojoy")) {
        // Abort startup if user requests no joystick.
        return;
    }
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        Con_Printf("Couldn't init game controller: %s\n", SDL_GetError());
        return;
    }
    SDL_GameControllerEventState(SDL_ENABLE);
}

void IN_ShutdownGamepad(void) {
    if (gamepad) {
        SDL_GameControllerClose(gamepad);
        gamepad = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

//==============================================================================
