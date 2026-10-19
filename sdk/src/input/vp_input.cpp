// The cabinet's controls: advmame.rc actions read through SDL2, and the
// functions a game names and lays out itself (vp_controls.h).
//
// Each action is a list of bindings ("or" alternatives) from advmame.rc, or
// AdvanceMAME's default for it when the file says "auto" or nothing. The key
// name table and the joystick-name matching follow dvg_utils' advmame_rc.cpp,
// which the menu and the games use against the same file. A game's function
// is an action too, its bindings read from its controls as text.

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

// What the snapshot of every control keeps (vp_controls.h).
const int RAW_JOYS    = 8;
const int RAW_BUTTONS = 32;
const int RAW_HATS    = 4;
const int RAW_AXES    = 16;
const int CENTRED     = 8000;    // an axis seen this near its middle is a stick's (not a pedal's)

// One binding as it is read: an advmame.rc one, or a control a game laid out.
struct Bind {
    vp_binding b{};
    int  scancode = -1;     // keys: resolved
    int  hatBit   = 0;      // a hat's way (SDL_HAT_UP...; b.index is the hat): only
                            // laid out by a game, advmame.rc has none
    bool yoke     = false;  // the device is the yoke, whatever its number
};

struct Action {
    std::vector<Bind> binds;
    bool  held = false, wasHeld = false;
    float axis = 0.0f;
    int   delta = 0;
    // A game's function (vp_input_define): its standard, and what the player set.
    bool  own = false;
    std::string standard, controls;
};

// Every control as it is: the keyboard, each joystick's buttons, hats and
// axes, the mouse's buttons.
struct Raw {
    Uint8  key[SDL_NUM_SCANCODES];
    Uint8  button[RAW_JOYS][RAW_BUTTONS];
    Uint8  hat[RAW_JOYS][RAW_HATS];
    Sint16 axis[RAW_JOYS][RAW_AXES];
    Uint32 mouse;
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
    // vp_controls.h: every control, this update and the last.
    Raw   now{}, last{};
    Uint8 centred[RAW_JOYS][RAW_AXES] = {};   // the axis has rested near its middle
    int   yoke = -1;                          // the first joystick named "...yoke...", or -1
    std::vector<int> reserved;                // keys the game keeps (vp_input_reserve_key)
    std::vector<std::string> own;             // the game's functions, in the order defined
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
    in->yoke = -1;
    for (size_t i = 0; i < in->joystickNames.size() && in->yoke < 0; i++)
        if (in->joystickNames[i].find("yoke") != std::string::npos) in->yoke = (int)i;
    memset(in->centred, 0, sizeof in->centred);
}

// advmame.rc names a joystick by number or by name; a game's control may
// name the yoke.
SDL_Joystick *joystickFor(const vp_input *in, const Bind &bd)
{
    if (bd.yoke)
        return in->yoke >= 0 ? in->joysticks[(size_t)in->yoke] : nullptr;
    const char *device = bd.b.device;
    if (device[0] && isdigit((unsigned char)device[0])) {
        size_t i = (size_t)atoi(device);
        return i < in->joysticks.size() ? in->joysticks[i] : nullptr;
    }
    for (size_t i = 0; i < in->joysticks.size(); i++)
        if (in->joystickNames[i] == device) return in->joysticks[i];
    return nullptr;
}

void setBindings(Action &a, const std::vector<vp_binding> &bindings)
{
    a.binds.clear();
    for (const vp_binding &b : bindings) {
        Bind bd;
        bd.b = b;
        if (b.type == VP_BINDING_KEY) bd.scancode = (int)keyCode(b.key);
        a.binds.push_back(bd);
    }
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
            if (!bindings.empty()) setBindings(in->actions[name], bindings);
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

    int nkeys = 0;
    const Uint8 *keys = SDL_GetKeyboardState(&nkeys);
    int dx = 0, dy = 0;
    Uint32 buttons = SDL_GetRelativeMouseState(&dx, &dy);
    in->mouseDx = dx;
    in->mouseDy = dy;

    // Every control, for the game's own (vp_controls.h).
    in->last = in->now;
    memset(&in->now, 0, sizeof in->now);
    if (keys) memcpy(in->now.key, keys, (size_t)(nkeys < SDL_NUM_SCANCODES ? nkeys : SDL_NUM_SCANCODES));
    for (size_t j = 0; j < in->joysticks.size() && (int)j < RAW_JOYS; j++) {
        SDL_Joystick *s = in->joysticks[j];
        if (!s) continue;
        int n = SDL_JoystickNumButtons(s);
        for (int k = 0; k < n && k < RAW_BUTTONS; k++) in->now.button[j][k] = SDL_JoystickGetButton(s, k) ? 1 : 0;
        n = SDL_JoystickNumHats(s);
        for (int k = 0; k < n && k < RAW_HATS; k++) in->now.hat[j][k] = SDL_JoystickGetHat(s, k);
        n = SDL_JoystickNumAxes(s);
        for (int k = 0; k < n && k < RAW_AXES; k++) {
            Sint16 v = SDL_JoystickGetAxis(s, k);
            in->now.axis[j][k] = v;
            if (v > -CENTRED && v < CENTRED) in->centred[j][k] = 1;
        }
    }
    in->now.mouse = buttons;

    for (auto &entry : in->actions) {
        Action &a = entry.second;
        a.wasHeld = a.held;
        bool held = false;
        float axis = 0.0f;
        int delta = 0;
        for (const Bind &bd : a.binds) {
            const vp_binding &b = bd.b;
            if (bd.hatBit) {
                SDL_Joystick *j = joystickFor(in, bd);
                if (j && (SDL_JoystickGetHat(j, b.index) & bd.hatBit)) held = true;
                continue;
            }
            switch (b.type) {
            case VP_BINDING_KEY:
                if (bd.scancode > 0 && bd.scancode < nkeys && keys[bd.scancode]) held = true;
                break;
            case VP_BINDING_JOY_BUTTON: {
                SDL_Joystick *j = joystickFor(in, bd);
                if (j && SDL_JoystickGetButton(j, b.index)) held = true;
                break;
            }
            case VP_BINDING_JOY_AXIS_NEG:
            case VP_BINDING_JOY_AXIS_POS: {
                SDL_Joystick *j = joystickFor(in, bd);
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
                SDL_Joystick *j = joystickFor(in, bd);
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

// ===========================================================================
// A game's own controls (vp_controls.h)
// ===========================================================================

namespace {

const char *const HAT_WAYS[4] = { "up", "right", "down", "left" };
const Uint8 HAT_BITS[4]       = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT };
// vp_binding's mouse buttons: 0 left, 1 right, 2 middle, 3 side, 4 extra.
const char *const MOUSE_NAMES[5] = { "left", "right", "middle", "x1", "x2" };
const int MOVED = 20000;        // how far from where it rested an axis must go to be laid out

// The '+' parts of controls as text.
std::vector<std::string> split(const char *controls)
{
    std::vector<std::string> out;
    const char *p = controls ? controls : "";
    while (*p) {
        size_t n = strcspn(p, "+");
        if (n) out.emplace_back(p, n);
        p += n;
        if (*p == '+') p++;
    }
    return out;
}

std::string joined(const std::vector<std::string> &parts)
{
    std::string out;
    for (const std::string &one : parts) out += (out.empty() ? "" : "+") + one;
    return out;
}

int countOf(const std::string &controls)
{
    return controls.empty() || controls == "none" ? 0 : (int)split(controls.c_str()).size();
}

// `one` added to `controls` unless it has it; the first goes past `max` (0: no limit).
std::string addOne(const std::string &controls, const std::string &one, int max)
{
    std::vector<std::string> parts = controls == "none" ? std::vector<std::string>() : split(controls.c_str());
    for (const std::string &p : parts)
        if (p == one) return joined(parts);
    while (max > 0 && (int)parts.size() >= max) parts.erase(parts.begin());
    parts.push_back(one);
    return joined(parts);
}

// "joy2_..." / "yoke_...": the device part of a control into `bd`, and what
// follows it. False when the control names no joystick.
bool deviceOf(const std::string &one, Bind &bd, std::string &rest)
{
    if (one.compare(0, 5, "yoke_") == 0) {
        bd.yoke = true;
        rest = one.substr(5);
        return true;
    }
    int j = 0, n = 0;
    if (sscanf(one.c_str(), "joy%d_%n", &j, &n) == 1 && n > 0 && j >= 1 && j <= RAW_JOYS) {
        snprintf(bd.b.device, sizeof bd.b.device, "%d", j - 1);
        rest = one.substr((size_t)n);
        return true;
    }
    return false;
}

// One control as bindings, appended to `out`. An advmame.rc action stands
// for its bindings; "standard" for `standard`'s. Unknown ones add nothing.
void parseOne(const vp_input *in, const std::string &one, const std::string &standard,
              std::vector<Bind> &out)
{
    if (one == "none" || one.empty()) return;
    if (one == "standard") {
        for (const std::string &s : split(standard.c_str()))
            if (s != "standard") parseOne(in, s, "", out);
        return;
    }
    Bind bd;
    if (one.compare(0, 4, "key_") == 0) {
        std::string name = one.substr(4);
        for (char &c : name) if (c == '_') c = ' ';
        SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
        if (sc == SDL_SCANCODE_UNKNOWN) return;
        bd.b.type = VP_BINDING_KEY;
        bd.scancode = (int)sc;
        out.push_back(bd);
        return;
    }
    std::string rest;
    if (deviceOf(one, bd, rest)) {
        int k = 0;
        char way[8] = "";
        if (sscanf(rest.c_str(), "button%d", &k) == 1 && k >= 1 && k <= RAW_BUTTONS) {
            bd.b.type = VP_BINDING_JOY_BUTTON;
            bd.b.index = k - 1;
        } else if (sscanf(rest.c_str(), "hat%d_%7s", &k, way) == 2 && k >= 1 && k <= RAW_HATS) {
            bd.b.index = k - 1;
            for (int w = 0; w < 4; w++)
                if (!strcmp(way, HAT_WAYS[w])) bd.hatBit = HAT_BITS[w];
            if (!bd.hatBit) return;
        } else if (sscanf(rest.c_str(), "axis%d_%7s", &k, way) == 2 && k >= 1 && k <= RAW_AXES) {
            if (strcmp(way, "neg") && strcmp(way, "pos")) return;
            bd.b.type = strcmp(way, "neg") ? VP_BINDING_JOY_AXIS_POS : VP_BINDING_JOY_AXIS_NEG;
            bd.b.index = k - 1;
        } else {
            return;
        }
        out.push_back(bd);
        return;
    }
    if (one.compare(0, 6, "mouse_") == 0) {
        for (int w = 0; w < 5; w++)
            if (one.compare(6, std::string::npos, MOUSE_NAMES[w]) == 0) {
                bd.b.type = VP_BINDING_MOUSE_BUTTON;
                bd.b.index = w;
                out.push_back(bd);
            }
        return;
    }
    const Action *a = findAction(in, one.c_str());     // an advmame.rc action
    if (a) out.insert(out.end(), a->binds.begin(), a->binds.end());
}

std::vector<Bind> parseControls(const vp_input *in, const char *controls, const std::string &standard)
{
    std::vector<Bind> out;
    for (const std::string &one : split(controls)) parseOne(in, one, standard, out);
    return out;
}

// A binding as a control's text ("joy1_button3"), or empty (analog ones).
std::string controlOf(const vp_input *in, const Bind &bd)
{
    const vp_binding &b = bd.b;
    std::string dev;
    if (bd.yoke) {
        dev = "yoke_";
    } else if (b.device[0] && isdigit((unsigned char)b.device[0])) {
        dev = "joy" + std::to_string(atoi(b.device) + 1) + "_";
    } else if (b.device[0]) {
        for (size_t i = 0; i < in->joystickNames.size() && dev.empty(); i++)
            if (in->joystickNames[i] == b.device) dev = "joy" + std::to_string(i + 1) + "_";
        if (dev.empty()) dev = std::string(b.device) + "_";
    }
    if (bd.hatBit) {
        for (int w = 0; w < 4; w++)
            if (bd.hatBit == HAT_BITS[w]) return dev + "hat" + std::to_string(b.index + 1) + "_" + HAT_WAYS[w];
        return std::string();
    }
    switch (b.type) {
    case VP_BINDING_KEY: {
        int sc = bd.scancode > 0 ? bd.scancode : (int)keyCode(b.key);
        const char *name = sc > 0 ? SDL_GetScancodeName((SDL_Scancode)sc) : "";
        std::string out = std::string("key_") + (name ? name : "");
        for (char &c : out) if (c == ' ') c = '_';
        return name && name[0] ? out : std::string();
    }
    case VP_BINDING_JOY_BUTTON:
        return dev + "button" + std::to_string(b.index + 1);
    case VP_BINDING_JOY_AXIS_NEG:
    case VP_BINDING_JOY_AXIS_POS:
        return dev + "axis" + std::to_string(b.index + 1) + (b.type == VP_BINDING_JOY_AXIS_NEG ? "_neg" : "_pos");
    case VP_BINDING_MOUSE_BUTTON:
        return b.index >= 0 && b.index < 5 ? std::string("mouse_") + MOUSE_NAMES[b.index] : std::string();
    default:
        return std::string();
    }
}

// "joy1_button3" -> "JOY1 BUTTON 3".
std::string spellOne(const std::string &one)
{
    char word[VP_CONTROLS_LENGTH] = "";
    char way[8] = "";
    int k = 0;
    Bind bd;
    std::string rest, dev;
    if (one.compare(0, 4, "key_") == 0) {
        snprintf(word, sizeof word, "%s", one.c_str() + 4);
    } else if (deviceOf(one, bd, rest)) {
        dev = bd.yoke ? "YOKE" : "JOY" + std::to_string(atoi(bd.b.device) + 1);
        if (sscanf(rest.c_str(), "button%d", &k) == 1)
            snprintf(word, sizeof word, "%s BUTTON %d", dev.c_str(), k);
        else if (sscanf(rest.c_str(), "hat%d_%7s", &k, way) == 2) {
            if (k == 1) snprintf(word, sizeof word, "%s HAT %s", dev.c_str(), way);
            else        snprintf(word, sizeof word, "%s HAT%d %s", dev.c_str(), k, way);
        }
        else if (sscanf(rest.c_str(), "axis%d_%7s", &k, way) == 2)
            snprintf(word, sizeof word, "%s AXIS %d %s", dev.c_str(), k, strcmp(way, "neg") ? "+" : "-");
        else
            snprintf(word, sizeof word, "%s", one.c_str());
    } else if (one.compare(0, 6, "mouse_") == 0) {
        snprintf(word, sizeof word, "MOUSE %s", one.c_str() + 6);
    } else {
        snprintf(word, sizeof word, "%s", one.c_str());
    }
    std::string out;
    for (const char *p = word; *p; p++) out += *p == '_' ? ' ' : (char)toupper((unsigned char)*p);
    return out;
}

Action *ownAction(vp_input *in, const char *action)
{
    if (!in || !action) return nullptr;
    auto it = in->actions.find(action);
    return it != in->actions.end() && it->second.own ? &it->second : nullptr;
}

const Action *ownAction(const vp_input *in, const char *action)
{
    return ownAction(const_cast<vp_input *>(in), action);
}

std::string trimmed(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string controlsPath(const char *path)
{
    return path ? std::string(path) : std::string(vp_save_dir()) + "/controls.ini";
}

}  // namespace

int vp_input_define(vp_input *in, const char *action, const char *standard)
{
    if (!in || !action || !action[0]) return vp_fail(VP_ERR_INVALID, "vp_input_define: NULL argument");
    std::vector<Bind> binds = parseControls(in, standard, "");    // before it may replace an action it names
    Action &a = in->actions[action];
    if (!a.own) in->own.push_back(action);
    a.own = true;
    a.standard = standard ? standard : "none";
    a.controls = "standard";
    a.binds = std::move(binds);
    a.held = a.wasHeld = false;
    return VP_OK;
}

int vp_input_set(vp_input *in, const char *action, const char *controls)
{
    Action *a = ownAction(in, action);
    if (!a) return vp_fail(VP_ERR_NOTFOUND, "vp_input_set: '%s' is not a function the game defined",
                           action ? action : "(null)");
    std::string c = controls ? trimmed(controls) : std::string("standard");
    if (c.empty()) c = "none";
    a->binds = parseControls(in, c.c_str(), a->standard);
    a->controls = c;
    return VP_OK;
}

const char *vp_input_controls(const vp_input *in, const char *action)
{
    const Action *a = ownAction(in, action);
    return a ? a->controls.c_str() : nullptr;
}

const char *vp_input_spelled(const vp_input *in, const char *action, const char *controls,
                             char *buf, size_t size)
{
    if (!buf || !size) return buf;
    const Action *a = ownAction(in, action);
    const std::string standard = a ? a->standard : std::string();
    std::string all;                    // the controls they stand for, each once
    for (const std::string &one : split(controls)) {
        if (one == "none") continue;
        Bind probe;
        std::string rest;
        bool plain = one.compare(0, 4, "key_") == 0 || one.compare(0, 6, "mouse_") == 0 || deviceOf(one, probe, rest);
        if (plain) { all = addOne(all, one, 0); continue; }
        std::vector<Bind> binds;
        parseOne(in, one, standard, binds);
        for (const Bind &bd : binds) {
            std::string c = controlOf(in, bd);
            if (!c.empty()) all = addOne(all, c, 0);
        }
    }
    std::string out;
    for (const std::string &one : split(all.c_str())) out += (out.empty() ? "" : " + ") + spellOne(one);
    vp_strlcpy(buf, out.empty() ? "NONE" : out.c_str(), size);
    return buf;
}

int vp_input_save(const vp_input *in, const char *path)
{
    if (!in) return vp_fail(VP_ERR_INVALID, "vp_input_save: NULL argument");
    std::string p = controlsPath(path);
    FILE *f = fopen(p.c_str(), "w");
    if (!f) return vp_fail(VP_ERR_IO, "cannot write %s", p.c_str());
    fprintf(f, "# The player's controls: function = controls (vp_controls.h)\n");
    for (const std::string &name : in->own) {
        const Action *a = ownAction(in, name.c_str());
        if (a) fprintf(f, "%s = %s\n", name.c_str(), a->controls.c_str());
    }
    int bad = ferror(f);
    if (fclose(f) != 0 || bad) return vp_fail(VP_ERR_IO, "write to %s failed", p.c_str());
    return VP_OK;
}

int vp_input_load(vp_input *in, const char *path)
{
    if (!in) return vp_fail(VP_ERR_INVALID, "vp_input_load: NULL argument");
    std::string p = controlsPath(path);
    FILE *f = fopen(p.c_str(), "r");
    if (!f) return vp_fail(VP_ERR_NOTFOUND, "%s does not exist", p.c_str());
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string l = trimmed(line);
        if (l.empty() || l[0] == '#' || l[0] == ';') continue;
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string name = trimmed(l.substr(0, eq)), value = trimmed(l.substr(eq + 1));
        if (ownAction(in, name.c_str())) vp_input_set(in, name.c_str(), value.c_str());
    }
    fclose(f);
    return VP_OK;
}

// ---- the menus' fixed controls --------------------------------------------

namespace {

const int PAST_HALF = 16384;    // an axis this far out is a way pressed

bool keyNow(const vp_input *in, int s)  { return s > 0 && s < SDL_NUM_SCANCODES && in->now.key[s]; }
bool keyLast(const vp_input *in, int s) { return s > 0 && s < SDL_NUM_SCANCODES && in->last.key[s]; }

// A way (a hat bit) on any joystick's hats or its first stick, now or the last update.
bool stickWay(const vp_input *in, int bit, bool last)
{
    const Raw &r = last ? in->last : in->now;
    for (size_t jj = 0; jj < in->joysticks.size() && (int)jj < RAW_JOYS; jj++) {
        int j = (int)jj;
        for (int k = 0; k < RAW_HATS; k++) if (r.hat[j][k] & bit) return true;
        if (bit == SDL_HAT_LEFT  && in->centred[j][0] && r.axis[j][0] < -PAST_HALF) return true;
        if (bit == SDL_HAT_RIGHT && in->centred[j][0] && r.axis[j][0] >  PAST_HALF) return true;
        if (bit == SDL_HAT_UP    && in->centred[j][1] && r.axis[j][1] < -PAST_HALF) return true;
        if (bit == SDL_HAT_DOWN  && in->centred[j][1] && r.axis[j][1] >  PAST_HALF) return true;
    }
    return false;
}

bool way(const vp_input *in, int key, int bit, bool last)
{
    return (last ? keyLast(in, key) : keyNow(in, key)) || stickWay(in, bit, last);
}

}  // namespace

int vp_input_key_held(const vp_input *in, int scancode)    { return in && keyNow(in, scancode); }
int vp_input_key_pressed(const vp_input *in, int scancode) { return in && keyNow(in, scancode) && !keyLast(in, scancode); }

int vp_input_menu_confirm(const vp_input *in)
{
    return in && (vp_input_pressed(in, "start1") || vp_input_key_pressed(in, SDL_SCANCODE_RETURN)
                  || vp_input_key_pressed(in, SDL_SCANCODE_KP_ENTER));
}

int vp_input_menu_confirm_held(const vp_input *in)
{
    return in && (vp_input_held(in, "start1") || keyNow(in, SDL_SCANCODE_RETURN) || keyNow(in, SDL_SCANCODE_KP_ENTER));
}

int vp_input_menu_up(const vp_input *in)
{
    return in && way(in, SDL_SCANCODE_UP, SDL_HAT_UP, false) && !way(in, SDL_SCANCODE_UP, SDL_HAT_UP, true);
}

int vp_input_menu_down(const vp_input *in)
{
    return in && way(in, SDL_SCANCODE_DOWN, SDL_HAT_DOWN, false) && !way(in, SDL_SCANCODE_DOWN, SDL_HAT_DOWN, true);
}

int vp_input_menu_left(const vp_input *in)  { return in && way(in, SDL_SCANCODE_LEFT, SDL_HAT_LEFT, false); }
int vp_input_menu_right(const vp_input *in) { return in && way(in, SDL_SCANCODE_RIGHT, SDL_HAT_RIGHT, false); }

int vp_input_menu_left_pressed(const vp_input *in)
{
    return vp_input_menu_left(in) && !way(in, SDL_SCANCODE_LEFT, SDL_HAT_LEFT, true);
}

int vp_input_menu_right_pressed(const vp_input *in)
{
    return vp_input_menu_right(in) && !way(in, SDL_SCANCODE_RIGHT, SDL_HAT_RIGHT, true);
}

int vp_input_menu_way_x(const vp_input *in) { return vp_input_menu_right(in) - vp_input_menu_left(in); }

int vp_input_menu_way_y(const vp_input *in)
{
    if (!in) return 0;
    return (int)way(in, SDL_SCANCODE_DOWN, SDL_HAT_DOWN, false) - (int)way(in, SDL_SCANCODE_UP, SDL_HAT_UP, false);
}

void vp_input_reserve_key(vp_input *in, int scancode)
{
    if (in && scancode > 0) in->reserved.push_back(scancode);
}

// ---- the joysticks as they are --------------------------------------------

int vp_input_yoke(const vp_input *in) { return in ? in->yoke : -1; }

int vp_input_stick(const vp_input *in)
{
    if (!in) return -1;
    for (size_t j = 0; j < in->joysticks.size() && (int)j < RAW_JOYS; j++)
        if ((int)j != in->yoke && in->centred[j][0] && in->centred[j][1]) return (int)j;
    return -1;
}

float vp_input_joy_axis(const vp_input *in, int joystick, int axis)
{
    if (!in || joystick < 0 || joystick >= RAW_JOYS || joystick >= (int)in->joysticks.size()
        || axis < 0 || axis >= RAW_AXES)
        return 0.0f;
    return (float)in->now.axis[joystick][axis] / 32767.0f;
}

// ---- laying out -----------------------------------------------------------

namespace {

bool reservedKey(const vp_input *in, int s)
{
    for (int r : in->reserved) if (s == r) return true;
    return s == SDL_SCANCODE_F2 || s == SDL_SCANCODE_ESCAPE || s == SDL_SCANCODE_RETURN
           || s == SDL_SCANCODE_KP_ENTER || s == SDL_SCANCODE_DELETE || s == SDL_SCANCODE_KP_PERIOD;
}

// A start, a coin, the way out or a reserved key held: nothing is taken.
bool reservedHeld(const vp_input *in)
{
    static const char *const CABINET[] = { "start1", "start2", "coin1", "coin2", "ui_cancel" };
    for (const char *c : CABINET) if (vp_input_held(in, c)) return true;
    for (int s = 1; s < SDL_NUM_SCANCODES; s++) if (in->now.key[s] && reservedKey(in, s)) return true;
    return false;
}

void restAxes(vp_capture *c, const vp_input *in)
{
    for (int j = 0; j < RAW_JOYS && j < 8; j++)
        for (int k = 0; k < RAW_AXES && k < 16; k++) c->rest[j][k] = in->now.axis[j][k];
}

// The control that has just gone down, or empty.
std::string fresh(const vp_capture *c, const vp_input *in)
{
    for (int s = SDL_SCANCODE_A; s < SDL_NUM_SCANCODES; s++) {
        if (!in->now.key[s] || in->last.key[s] || reservedKey(in, s)) continue;
        const char *name = SDL_GetScancodeName((SDL_Scancode)s);
        if (!name || !name[0]) continue;
        std::string one = std::string("key_") + name;
        for (char &ch : one) if (ch == ' ') ch = '_';
        return one;
    }
    for (size_t jj = 0; jj < in->joysticks.size() && (int)jj < RAW_JOYS; jj++) {
        int j = (int)jj;
        std::string dev = j == in->yoke ? std::string("yoke_") : "joy" + std::to_string(j + 1) + "_";
        for (int k = 0; k < RAW_BUTTONS; k++)
            if (in->now.button[j][k] && !in->last.button[j][k]) return dev + "button" + std::to_string(k + 1);
        for (int k = 0; k < RAW_HATS; k++)
            for (int w = 0; w < 4; w++)
                if ((in->now.hat[j][k] & HAT_BITS[w]) && !(in->last.hat[j][k] & HAT_BITS[w]))
                    return dev + "hat" + std::to_string(k + 1) + "_" + HAT_WAYS[w];
        for (int k = 0; k < RAW_AXES; k++) {
            int v = in->now.axis[j][k], from = c->rest[j][k];
            if (abs(v - from) < MOVED || (v < PAST_HALF && v > -PAST_HALF)) continue;
            return dev + "axis" + std::to_string(k + 1) + (v < 0 ? "_neg" : "_pos");
        }
    }
    for (int w = 0; w < 5; w++) {
        Uint32 m = mouseMask(w);
        if ((in->now.mouse & m) && !(in->last.mouse & m)) return std::string("mouse_") + MOUSE_NAMES[w];
    }
    return std::string();
}

void say(vp_capture *c, int frames, const char *what)
{
    c->said_frames = frames;
    vp_strlcpy(c->said, what, sizeof c->said);
}

// The capture is over: what was taken becomes the function's controls.
void done(vp_capture *c, vp_input *in)
{
    c->capturing = 0;
    c->let_go = 1;                      // what was pressed may be left or right as well
    if (!c->taken[0]) vp_strlcpy(c->taken, "none", sizeof c->taken);
    vp_input_set(in, c->action, c->taken);
    c->adding = strcmp(c->taken, "none") != 0;
}

}  // namespace

void vp_capture_moved(vp_capture *c) { if (c) c->adding = 0; }

void vp_capture_begin(vp_capture *c, const vp_input *in, const char *action)
{
    if (!c || !in || !action) return;
    vp_strlcpy(c->action, action, sizeof c->action);
    c->capturing = 1;
    c->armed = 0;
    c->taken[0] = 0;
    const char *has = vp_input_controls(in, action);
    if (c->adding && has && strcmp(has, "none")) vp_strlcpy(c->taken, has, sizeof c->taken);   // else the first replaces
    c->count = countOf(c->taken);
    restAxes(c, in);
}

void vp_capture_standard(vp_capture *c, vp_input *in, const char *action)
{
    if (!c || !in || !action) return;
    vp_strlcpy(c->action, action, sizeof c->action);
    vp_strlcpy(c->taken, "standard", sizeof c->taken);
    done(c, in);
    c->adding = 0;                      // the next replaces it again
}

void vp_capture_toggle(vp_capture *c, vp_input *in, const char *action)
{
    if (!c || !in || !action) return;
    const char *has = vp_input_controls(in, action);
    if (has && !strcmp(has, "none")) { vp_capture_standard(c, in, action); return; }
    vp_input_set(in, action, "none");
    c->adding = 0;
}

int vp_capture_step(vp_capture *c, vp_input *in)
{
    if (!c || !in || !c->capturing) return VP_CAPTURE_WAITING;
    if (!c->armed) {                    // the press that took the row must end first
        c->armed = !vp_input_menu_confirm_held(in);
        restAxes(c, in);
        return VP_CAPTURE_WAITING;
    }
    if (vp_input_menu_confirm(in)) { c->capturing = 0; return VP_CAPTURE_LEFT; }
    if (vp_input_pressed(in, "start2")) {
        vp_strlcpy(c->taken, "none", sizeof c->taken);
        done(c, in);
        return VP_CAPTURE_CHANGED;
    }
    if (vp_input_key_pressed(in, SDL_SCANCODE_DELETE) || vp_input_key_pressed(in, SDL_SCANCODE_KP_PERIOD)) {
        vp_capture_standard(c, in, c->action);
        return VP_CAPTURE_CHANGED;
    }
    if (reservedHeld(in)) {             // a start, a coin, the way out ... is not taken
        if (!fresh(c, in).empty()) say(c, 90, "START, COIN, ENTER AND EXIT STAY THE MENU'S");
        return VP_CAPTURE_WAITING;
    }
    std::string one = fresh(c, in);
    if (one.empty()) return VP_CAPTURE_WAITING;
    vp_strlcpy(c->taken, addOne(c->taken, one, VP_CONTROLS_MAX).c_str(), sizeof c->taken);
    done(c, in);
    return VP_CAPTURE_CHANGED;
}

void vp_capture_filter(vp_capture *c, int *left, int *right)
{
    if (!c || !left || !right || !c->let_go) return;
    c->let_go = *left || *right;
    *left = *right = 0;
}

const char *vp_capture_said(vp_capture *c)
{
    if (!c || c->said_frames <= 0) return nullptr;
    c->said_frames--;
    return c->said;
}

void vp_capture_help(const vp_capture *c, const char **one, const char **two)
{
    if (!one || !two) return;
    if (c && c->capturing) {
        *one = c->count ? "PRESS THE BUTTON TO ADD" : "PRESS THE BUTTON";
        *two = "START 1 - BACK  DEL - STANDARD  START 2 - NONE";
    } else {
        *one = "LEFT, RIGHT - NONE / STANDARD";
        *two = "START, ENTER - ADD BUTTON   DEL - STANDARD";
    }
}
