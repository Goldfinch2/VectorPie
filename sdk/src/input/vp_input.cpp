// The cabinet's controls: advmame.rc actions read through SDL2.
//
// Each action is a list of bindings ("or" alternatives) from advmame.rc, or
// AdvanceMAME's default for it when the file says "auto" or nothing. The key
// name table and the joystick-name matching follow dvg_utils' advmame_rc.cpp,
// which the menu and the games use against the same file.

#include "../core/vp_internal.h"
#include "../dvg/vp_window.h"

#include <SDL.h>

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// AdvanceMAME's defaults (MAME's): used for every action advmame.rc leaves at
// "auto" or does not list. Written in advmame.rc syntax.
// ---------------------------------------------------------------------------

struct DefaultBinding { const char *action; const char *bindings; };

const DefaultBinding DEFAULTS[] = {
    {"p1_up",      "keyboard[0,up] or joystick_digital[0,0,1,up]"},
    {"p1_down",    "keyboard[0,down] or joystick_digital[0,0,1,down]"},
    {"p1_left",    "keyboard[0,left] or joystick_digital[0,0,0,left]"},
    {"p1_right",   "keyboard[0,right] or joystick_digital[0,0,0,right]"},
    {"p1_button1", "keyboard[0,lcontrol] or joystick_button[0,button1] or mouse_button[0,left]"},
    {"p1_button2", "keyboard[0,lalt] or joystick_button[0,button2] or mouse_button[0,right]"},
    {"p1_button3", "keyboard[0,space] or joystick_button[0,button3] or mouse_button[0,middle]"},
    {"p1_button4", "keyboard[0,lshift] or joystick_button[0,button4]"},
    {"p1_button5", "keyboard[0,z] or joystick_button[0,button5]"},
    {"p1_button6", "keyboard[0,x] or joystick_button[0,button6]"},
    {"p2_up",      "keyboard[0,r] or joystick_digital[1,0,1,up]"},
    {"p2_down",    "keyboard[0,f] or joystick_digital[1,0,1,down]"},
    {"p2_left",    "keyboard[0,d] or joystick_digital[1,0,0,left]"},
    {"p2_right",   "keyboard[0,g] or joystick_digital[1,0,0,right]"},
    {"p2_button1", "keyboard[0,a] or joystick_button[1,button1]"},
    {"p2_button2", "keyboard[0,s] or joystick_button[1,button2]"},
    {"p2_button3", "keyboard[0,q] or joystick_button[1,button3]"},
    {"p2_button4", "keyboard[0,w] or joystick_button[1,button4]"},
    {"start1",     "keyboard[0,1]"},
    {"start2",     "keyboard[0,2]"},
    {"coin1",      "keyboard[0,5]"},
    {"coin2",      "keyboard[0,6]"},
    {"ui_cancel",  "keyboard[0,esc]"},
    {"ui_pause",   "keyboard[0,p]"},
    {"p1_stickx",  "joystick[0,0,x]"},
    {"p1_sticky",  "joystick[0,0,y]"},
    {"p2_stickx",  "joystick[1,0,x]"},
    {"p2_sticky",  "joystick[1,0,y]"},
    {"p1_dialx",       "mouse[0,x]"},
    {"p1_trackballx",  "mouse[0,x]"},
    {"p1_trackbally",  "mouse[0,y]"},
    {"p1_mousex",      "mouse[0,x]"},
    {"p1_mousey",      "mouse[0,y]"},
};

// ---------------------------------------------------------------------------
// AdvanceMAME key names -> SDL scancodes
// ---------------------------------------------------------------------------

struct KeyEntry { const char *name; SDL_Scancode code; };

const KeyEntry KEYS[] = {
    {"a", SDL_SCANCODE_A}, {"b", SDL_SCANCODE_B}, {"c", SDL_SCANCODE_C},
    {"d", SDL_SCANCODE_D}, {"e", SDL_SCANCODE_E}, {"f", SDL_SCANCODE_F},
    {"g", SDL_SCANCODE_G}, {"h", SDL_SCANCODE_H}, {"i", SDL_SCANCODE_I},
    {"j", SDL_SCANCODE_J}, {"k", SDL_SCANCODE_K}, {"l", SDL_SCANCODE_L},
    {"m", SDL_SCANCODE_M}, {"n", SDL_SCANCODE_N}, {"o", SDL_SCANCODE_O},
    {"p", SDL_SCANCODE_P}, {"q", SDL_SCANCODE_Q}, {"r", SDL_SCANCODE_R},
    {"s", SDL_SCANCODE_S}, {"t", SDL_SCANCODE_T}, {"u", SDL_SCANCODE_U},
    {"v", SDL_SCANCODE_V}, {"w", SDL_SCANCODE_W}, {"x", SDL_SCANCODE_X},
    {"y", SDL_SCANCODE_Y}, {"z", SDL_SCANCODE_Z},
    {"0", SDL_SCANCODE_0}, {"n0", SDL_SCANCODE_0},
    {"1", SDL_SCANCODE_1}, {"2", SDL_SCANCODE_2}, {"3", SDL_SCANCODE_3},
    {"4", SDL_SCANCODE_4}, {"5", SDL_SCANCODE_5}, {"6", SDL_SCANCODE_6},
    {"7", SDL_SCANCODE_7}, {"8", SDL_SCANCODE_8}, {"9", SDL_SCANCODE_9},
    {"f1", SDL_SCANCODE_F1}, {"f2", SDL_SCANCODE_F2}, {"f3", SDL_SCANCODE_F3},
    {"f4", SDL_SCANCODE_F4}, {"f5", SDL_SCANCODE_F5}, {"f6", SDL_SCANCODE_F6},
    {"f7", SDL_SCANCODE_F7}, {"f8", SDL_SCANCODE_F8}, {"f9", SDL_SCANCODE_F9},
    {"f10", SDL_SCANCODE_F10}, {"f11", SDL_SCANCODE_F11}, {"f12", SDL_SCANCODE_F12},
    {"enter", SDL_SCANCODE_RETURN}, {"esc", SDL_SCANCODE_ESCAPE},
    {"backspace", SDL_SCANCODE_BACKSPACE}, {"tab", SDL_SCANCODE_TAB},
    {"space", SDL_SCANCODE_SPACE},
    {"minus", SDL_SCANCODE_MINUS}, {"equals", SDL_SCANCODE_EQUALS},
    {"openbrace", SDL_SCANCODE_LEFTBRACKET}, {"closebrace", SDL_SCANCODE_RIGHTBRACKET},
    {"backslash", SDL_SCANCODE_BACKSLASH},
    {"semicolon", SDL_SCANCODE_SEMICOLON}, {"quote", SDL_SCANCODE_APOSTROPHE},
    {"backquote", SDL_SCANCODE_GRAVE}, {"grave", SDL_SCANCODE_GRAVE},
    {"comma", SDL_SCANCODE_COMMA}, {"period", SDL_SCANCODE_PERIOD},
    {"slash", SDL_SCANCODE_SLASH},
    {"insert", SDL_SCANCODE_INSERT}, {"del", SDL_SCANCODE_DELETE},
    {"home", SDL_SCANCODE_HOME}, {"end", SDL_SCANCODE_END},
    {"pgup", SDL_SCANCODE_PAGEUP}, {"pgdn", SDL_SCANCODE_PAGEDOWN},
    {"left", SDL_SCANCODE_LEFT}, {"right", SDL_SCANCODE_RIGHT},
    {"up", SDL_SCANCODE_UP}, {"down", SDL_SCANCODE_DOWN},
    {"lshift", SDL_SCANCODE_LSHIFT}, {"rshift", SDL_SCANCODE_RSHIFT},
    {"lcontrol", SDL_SCANCODE_LCTRL}, {"rcontrol", SDL_SCANCODE_RCTRL},
    {"lalt", SDL_SCANCODE_LALT}, {"ralt", SDL_SCANCODE_RALT},
    {"lwin", SDL_SCANCODE_LGUI}, {"rwin", SDL_SCANCODE_RGUI},
    {"capslock", SDL_SCANCODE_CAPSLOCK}, {"numlock", SDL_SCANCODE_NUMLOCKCLEAR},
    {"scrlock", SDL_SCANCODE_SCROLLLOCK},
    {"prtscr", SDL_SCANCODE_PRINTSCREEN}, {"pause", SDL_SCANCODE_PAUSE},
    {"menu", SDL_SCANCODE_APPLICATION},
    {"0_pad", SDL_SCANCODE_KP_0}, {"1_pad", SDL_SCANCODE_KP_1},
    {"2_pad", SDL_SCANCODE_KP_2}, {"3_pad", SDL_SCANCODE_KP_3},
    {"4_pad", SDL_SCANCODE_KP_4}, {"5_pad", SDL_SCANCODE_KP_5},
    {"6_pad", SDL_SCANCODE_KP_6}, {"7_pad", SDL_SCANCODE_KP_7},
    {"8_pad", SDL_SCANCODE_KP_8}, {"9_pad", SDL_SCANCODE_KP_9},
    {"period_pad", SDL_SCANCODE_KP_PERIOD}, {"enter_pad", SDL_SCANCODE_KP_ENTER},
    {"slash_pad", SDL_SCANCODE_KP_DIVIDE}, {"asterisk_pad", SDL_SCANCODE_KP_MULTIPLY},
    {"minus_pad", SDL_SCANCODE_KP_MINUS}, {"plus_pad", SDL_SCANCODE_KP_PLUS},
};

SDL_Scancode keyCode(const char *name)
{
    for (const KeyEntry &k : KEYS)
        if (strcmp(name, k.name) == 0) return k.code;
    return SDL_SCANCODE_UNKNOWN;
}

// vp_binding mouse buttons are 0 left, 1 right, 2 middle, 3 side, 4 extra.
Uint32 mouseMask(int index)
{
    static const int SDL_BUTTONS[] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE,
                                       SDL_BUTTON_X1, SDL_BUTTON_X2 };
    if (index < 0 || index > 4) return 0;
    return SDL_BUTTON(SDL_BUTTONS[index]);
}

// AdvanceMAME's device_trim_name(), as vp_rc normalizes the names it reads.
std::string normalizeName(const char *s)
{
    std::string out;
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (isspace(c) || c == '_') out += '_';
        else if (isalnum(c)) out += (char)tolower(c);
    }
    size_t a = out.find_first_not_of('_');
    if (a == std::string::npos) return "";
    size_t b = out.find_last_not_of('_');
    return out.substr(a, b - a + 1);
}

// A digital joystick direction counts past half travel, as AdvanceMAME does.
const int DIGITAL_THRESHOLD = 16000;

struct Action {
    std::vector<vp_binding> bindings;
    std::vector<int> scancodes;     // per binding: resolved key, or -1
    bool  held = false, wasHeld = false;
    float axis = 0.0f;
    int   delta = 0;
};

}  // namespace

struct vp_input {
    std::unordered_map<std::string, Action> actions;
    std::vector<SDL_Joystick *> joysticks;
    std::vector<std::string> joystickNames;    // normalized, per open joystick
    int   sdlJoystickCount = -1;
    float deadzone = 0.15f;
    int   mouseDx = 0, mouseDy = 0;
    bool  wantGrab = false;     // vp_input_grab_mouse: relative mouse mode wanted
};

namespace {

void closeJoysticks(vp_input *in)
{
    for (SDL_Joystick *j : in->joysticks)
        if (j) SDL_JoystickClose(j);
    in->joysticks.clear();
    in->joystickNames.clear();
}

void openJoysticks(vp_input *in)
{
    closeJoysticks(in);
    int n = SDL_NumJoysticks();
    for (int i = 0; i < n; i++) {
        SDL_Joystick *j = SDL_JoystickOpen(i);
        in->joysticks.push_back(j);
        const char *name = j ? SDL_JoystickName(j) : nullptr;
        in->joystickNames.push_back(name ? normalizeName(name) : std::string());
    }
    in->sdlJoystickCount = n;
}

// advmame.rc names a joystick by number or by name.
SDL_Joystick *joystickFor(const vp_input *in, const char *device)
{
    if (device[0] && isdigit((unsigned char)device[0])) {
        size_t i = (size_t)atoi(device);
        return i < in->joysticks.size() ? in->joysticks[i] : nullptr;
    }
    for (size_t i = 0; i < in->joysticks.size(); i++)
        if (in->joystickNames[i] == device) return in->joysticks[i];
    return nullptr;
}

void setBindings(Action &a, std::vector<vp_binding> bindings)
{
    a.bindings = std::move(bindings);
    a.scancodes.assign(a.bindings.size(), -1);
    for (size_t i = 0; i < a.bindings.size(); i++)
        if (a.bindings[i].type == VP_BINDING_KEY)
            a.scancodes[i] = (int)keyCode(a.bindings[i].key);
}

const Action *findAction(const vp_input *in, const char *name)
{
    if (!in || !name) return nullptr;
    auto it = in->actions.find(name);
    return it == in->actions.end() ? nullptr : &it->second;
}

}  // namespace

int vp_input_open(vp_input **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp_input_open: NULL argument");
    *out = nullptr;

    // Joysticks must keep reporting when no window has the focus (a board
    // output has no window at all).
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    int r = vp_sdl_video_init();     // the keyboard comes through video
    if (r != VP_OK) return r;
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        r = vp_fail(VP_ERR_NODEV, "SDL joystick init failed: %s", SDL_GetError());
        vp_sdl_video_quit();
        return r;
    }

    vp_input *in = new (std::nothrow) vp_input;
    if (!in) {
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
        vp_sdl_video_quit();
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }

    // Defaults first, then advmame.rc on top: an action it binds replaces the
    // default; "auto" and missing actions keep it.
    for (const DefaultBinding &d : DEFAULTS)
        setBindings(in->actions[d.action], vp_rc_parse_bindings(d.bindings));

    vp_rc *rc = nullptr;
    if (vp_rc_load(nullptr, &rc) == VP_OK) {
        for (int i = 0; i < vp_rc_action_count(rc); i++) {
            const char *name = vp_rc_action_name(rc, i);
            std::vector<vp_binding> bindings;
            for (int j = 0; j < vp_rc_binding_count(rc, name); j++) {
                vp_binding b;
                if (vp_rc_binding(rc, name, j, &b) == VP_OK) bindings.push_back(b);
            }
            if (!bindings.empty()) setBindings(in->actions[name], std::move(bindings));
        }
        vp_rc_free(rc);
    }

    openJoysticks(in);
    SDL_GetRelativeMouseState(nullptr, nullptr);   // start counting from now
    *out = in;
    return VP_OK;
}

void vp_input_close(vp_input *in)
{
    if (!in) return;
    // Release a grab before the program ends: left captured, the display
    // can keep the keyboard from the next program (the menu).
    if (in->wantGrab) SDL_SetRelativeMouseMode(SDL_FALSE);
    closeJoysticks(in);
    delete in;
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
    vp_sdl_video_quit();
}

void vp_input_update(vp_input *in)
{
    if (!in) return;
    SDL_PumpEvents();
    // A grab asked for before there was a window (the marquee's, the
    // game's) takes effect once there is one.
    if (in->wantGrab && !SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_JoystickUpdate();
    if (SDL_NumJoysticks() != in->sdlJoystickCount)
        openJoysticks(in);

    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
    int dx = 0, dy = 0;
    Uint32 buttons = SDL_GetRelativeMouseState(&dx, &dy);
    in->mouseDx = dx;
    in->mouseDy = dy;

    for (auto &entry : in->actions) {
        Action &a = entry.second;
        a.wasHeld = a.held;
        bool held = false;
        float axis = 0.0f;
        int delta = 0;
        for (size_t i = 0; i < a.bindings.size(); i++) {
            const vp_binding &b = a.bindings[i];
            switch (b.type) {
            case VP_BINDING_KEY:
                if (a.scancodes[i] > 0 && keys[a.scancodes[i]]) held = true;
                break;
            case VP_BINDING_JOY_BUTTON: {
                SDL_Joystick *j = joystickFor(in, b.device);
                if (j && SDL_JoystickGetButton(j, b.index)) held = true;
                break;
            }
            case VP_BINDING_JOY_AXIS_NEG:
            case VP_BINDING_JOY_AXIS_POS: {
                SDL_Joystick *j = joystickFor(in, b.device);
                if (!j) break;
                int v = SDL_JoystickGetAxis(j, b.index);
                if (b.type == VP_BINDING_JOY_AXIS_NEG ? v < -DIGITAL_THRESHOLD : v > DIGITAL_THRESHOLD)
                    held = true;
                break;
            }
            case VP_BINDING_MOUSE_BUTTON:
                if (buttons & mouseMask(b.index)) held = true;
                break;
            case VP_BINDING_JOY_ANALOG: {
                SDL_Joystick *j = joystickFor(in, b.device);
                if (!j) break;
                float v = SDL_JoystickGetAxis(j, b.index) / 32767.0f;
                if (v < -1.0f) v = -1.0f;
                // Dead zone, then rescale so the axis still reaches 1.
                float m = std::fabs(v);
                v = m <= in->deadzone ? 0.0f
                                      : (v > 0 ? 1.0f : -1.0f) * (m - in->deadzone) / (1.0f - in->deadzone);
                axis += b.invert ? -v : v;
                break;
            }
            case VP_BINDING_MOUSE_AXIS: {
                int m = b.index == 0 ? dx : dy;
                delta += b.invert ? -m : m;
                break;
            }
            }
        }
        if (axis > 1.0f) axis = 1.0f;
        if (axis < -1.0f) axis = -1.0f;
        a.held = held;
        a.axis = axis;
        a.delta = delta;
    }
}

int vp_input_held(const vp_input *in, const char *action)
{
    const Action *a = findAction(in, action);
    return a && a->held;
}

int vp_input_pressed(const vp_input *in, const char *action)
{
    const Action *a = findAction(in, action);
    return a && a->held && !a->wasHeld;
}

int vp_input_released(const vp_input *in, const char *action)
{
    const Action *a = findAction(in, action);
    return a && !a->held && a->wasHeld;
}

float vp_input_axis(const vp_input *in, const char *action)
{
    const Action *a = findAction(in, action);
    return a ? a->axis : 0.0f;
}

int vp_input_delta(const vp_input *in, const char *action)
{
    const Action *a = findAction(in, action);
    return a ? a->delta : 0;
}

void vp_input_set_deadzone(vp_input *in, float fraction)
{
    if (!in) return;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 0.95f) fraction = 0.95f;
    in->deadzone = fraction;
}

int vp_input_joystick_count(const vp_input *in)
{
    return in ? (int)in->joysticks.size() : 0;
}

void vp_input_grab_mouse(vp_input *in, int grab)
{
    if (!in) return;
    in->wantGrab = grab != 0;
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
    SDL_GetRelativeMouseState(nullptr, nullptr);   // count from now
}
