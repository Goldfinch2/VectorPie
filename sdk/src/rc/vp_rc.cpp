// advmame.rc reader. The binding grammar follows dvg_utils' advmame_rc.cpp,
// which the menu and the games use against AdvanceMAME's own files; device
// names are kept as written (no SDL here), and are matched to real devices by
// the input layer.

#include "../core/vp_internal.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

struct Entry {
    std::string key;
    std::string value;
};

struct Action {
    std::string name;
    std::vector<vp_binding> bindings;
};

struct vp_rc {
    std::string path;
    std::vector<Entry> entries;     // last occurrence of each key wins
    std::vector<Action> actions;    // file order
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool isNumber(const std::string &s)
{
    if (s.empty()) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); i++)
        if (!isdigit((unsigned char)s[i])) return false;
    return true;
}

// AdvanceMAME's device_trim_name(): lowercase, spaces and underscores become
// one underscore each, other punctuation is dropped, outer underscores trimmed.
static std::string normalizeName(const std::string &s)
{
    std::string out;
    for (char c : s) {
        if (isspace((unsigned char)c) || c == '_') out += '_';
        else if (isalnum((unsigned char)c)) out += (char)tolower((unsigned char)c);
    }
    size_t a = out.find_first_not_of('_');
    if (a == std::string::npos) return "";
    size_t b = out.find_last_not_of('_');
    return out.substr(a, b - a + 1);
}

static std::string deviceToken(const std::string &s)
{
    std::string t = trim(s);
    if (t.empty()) return "0";
    return isNumber(t) ? t : normalizeName(t);
}

// Button names. AdvanceMAME's "buttonN" is 1-based; the SDK is 0-based.
struct NamedIndex { const char *name; int index; };

static const NamedIndex JOY_BUTTONS[] = {
    {"trigger", 0}, {"top", 1}, {"thumb", 2}, {"thumb2", 3},
    {"top2", 4}, {"pinkie", 5}, {"base", 6}, {"base2", 7},
    {"base3", 8}, {"base4", 9}, {"base5", 10}, {"base6", 11},
    {"a", 0}, {"b", 1}, {"c", 2}, {"x", 3}, {"y", 4}, {"z", 5},
    {"tl", 6}, {"tr", 7}, {"tl2", 8}, {"tr2", 9},
    {"select", 10}, {"start", 11}, {"mode", 12},
    {nullptr, 0}
};

static const NamedIndex MOUSE_BUTTONS[] = {
    {"left", 0}, {"right", 1}, {"middle", 2}, {"side", 3}, {"extra", 4},
    {nullptr, 0}
};

static bool buttonIndex(const NamedIndex *table, const std::string &name, int *out)
{
    for (int i = 0; table[i].name; i++)
        if (name == table[i].name) { *out = table[i].index; return true; }
    if (name.size() > 6 && name.compare(0, 6, "button") == 0 && isNumber(name.substr(6))) {
        int n = atoi(name.c_str() + 6);
        if (n >= 1) { *out = n - 1; return true; }
    }
    return false;
}

// Split "a,b,c" into trimmed fields.
static std::vector<std::string> fields(const std::string &s)
{
    std::vector<std::string> out;
    size_t pos = 0;
    for (;;) {
        size_t c = s.find(',', pos);
        out.push_back(trim(s.substr(pos, c == std::string::npos ? std::string::npos : c - pos)));
        if (c == std::string::npos) break;
        pos = c + 1;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Binding parser
// ---------------------------------------------------------------------------

static bool parseBinding(const std::string &token, vp_binding *b)
{
    memset(b, 0, sizeof(*b));
    std::string t = trim(token);
    if (t.compare(0, 3, "not") == 0) return false;
    int invert = 0;
    if (!t.empty() && t[0] == '-') {
        invert = 1;
        t = trim(t.substr(1));
    }

    size_t open = t.find('[');
    size_t close = t.find(']');
    if (open == std::string::npos || close == std::string::npos || close < open) return false;

    std::string kind = t.substr(0, open);
    std::vector<std::string> f = fields(t.substr(open + 1, close - open - 1));

    if (kind == "keyboard") {
        if (f.size() < 2 || f[1].empty() || f[1].size() >= sizeof(b->key)) return false;
        b->type = VP_BINDING_KEY;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        vp_strlcpy(b->key, f[1].c_str(), sizeof(b->key));
        return true;
    }
    if (kind == "joystick_button") {
        if (f.size() < 2 || !buttonIndex(JOY_BUTTONS, f[1], &b->index)) return false;
        b->type = VP_BINDING_JOY_BUTTON;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        return true;
    }
    if (kind == "joystick_digital") {
        // joystick_digital[device,stick,axis,direction]
        if (f.size() < 4) return false;
        int stick = isNumber(f[1]) ? atoi(f[1].c_str()) : 0;
        int axis;
        if (f[2] == "x")      axis = stick * 2;
        else if (f[2] == "y") axis = stick * 2 + 1;
        else if (f[2] == "z") axis = 2;
        else if (isNumber(f[2])) axis = atoi(f[2].c_str());
        else return false;
        // AdvanceMAME: up/left = 1, down/right = 0. Up/left is the negative
        // side of the axis.
        const std::string &dir = f[3];
        bool negative = (dir == "up" || dir == "left" || dir == "1");
        b->type = negative ? VP_BINDING_JOY_AXIS_NEG : VP_BINDING_JOY_AXIS_POS;
        b->index = axis;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        return true;
    }
    if (kind == "mouse_button") {
        if (f.size() < 2 || !buttonIndex(MOUSE_BUTTONS, f[1], &b->index)) return false;
        b->type = VP_BINDING_MOUSE_BUTTON;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        return true;
    }
    if (kind == "mouse") {
        // mouse[device,axis]: the axis as x/y or as its number.
        if (f.size() < 2) return false;
        if (f[1] == "x" || f[1] == "0")      b->index = 0;
        else if (f[1] == "y" || f[1] == "1") b->index = 1;
        else return false;
        b->type = VP_BINDING_MOUSE_AXIS;
        b->invert = invert;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        return true;
    }
    if (kind == "joystick") {
        // joystick[device,stick,axis]: an analog axis, numbered like
        // joystick_digital's.
        if (f.size() < 3) return false;
        int stick = isNumber(f[1]) ? atoi(f[1].c_str()) : 0;
        int axis;
        if (f[2] == "x")      axis = stick * 2;
        else if (f[2] == "y") axis = stick * 2 + 1;
        else if (f[2] == "z") axis = 2;
        else if (isNumber(f[2])) axis = stick * 2 + atoi(f[2].c_str());
        else return false;
        b->type = VP_BINDING_JOY_ANALOG;
        b->index = axis;
        b->invert = invert;
        vp_strlcpy(b->device, deviceToken(f[0]).c_str(), sizeof(b->device));
        return true;
    }
    return false;
}

static std::vector<vp_binding> parseBindings(const std::string &value)
{
    std::vector<vp_binding> out;
    size_t pos = 0;
    while (pos < value.size()) {
        size_t orPos = value.find(" or ", pos);
        std::string part = trim(value.substr(pos, orPos == std::string::npos
                                                      ? std::string::npos : orPos - pos));
        pos = (orPos == std::string::npos) ? value.size() : orPos + 4;
        if (part.empty()) continue;
        // Split a part into its bracketed inputs ("keyboard[0,a] keyboard[0,b]").
        std::vector<std::string> inputs;
        size_t p = 0;
        while (p < part.size()) {
            size_t close = part.find(']', p);
            if (close == std::string::npos) break;
            std::string one = trim(part.substr(p, close - p + 1));
            if (!one.empty()) inputs.push_back(one);
            p = close + 1;
        }
        if (inputs.empty()) continue;
        // Analog inputs written together are added up by AdvanceMAME: keep
        // them all. A combination of buttons: keep its first element.
        std::string first = inputs[0];
        if (!first.empty() && first[0] == '-') first = first.substr(1);
        bool analog = first.compare(0, 9, "joystick[") == 0 || first.compare(0, 6, "mouse[") == 0;
        size_t n = analog ? inputs.size() : 1;
        for (size_t i = 0; i < n; i++) {
            vp_binding b;
            if (parseBinding(inputs[i], &b)) out.push_back(b);
        }
    }
    return out;
}

std::vector<vp_binding> vp_rc_parse_bindings(const std::string &value)
{
    return parseBindings(trim(value));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int vp_rc_default_path(char *path, size_t path_size)
{
    if (!path || path_size == 0)
        return vp_fail(VP_ERR_INVALID, "no buffer for the path");

    const char *env = getenv("VP_ADVMAME_RC");
    if (env && *env) {
        vp_strlcpy(path, env, path_size);
        return VP_OK;
    }

    std::string candidate;
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
    if (home && *home) candidate = std::string(home) + "\\.advance\\advmame.rc";
#else
    const char *home = getenv("HOME");
    if (home && *home) candidate = std::string(home) + "/.advance/advmame.rc";
#endif
    if (!candidate.empty()) {
        FILE *f = fopen(candidate.c_str(), "r");
        if (f) {
            fclose(f);
            vp_strlcpy(path, candidate.c_str(), path_size);
            return VP_OK;
        }
    }

#ifndef _WIN32
    // VectorPie's games run as user pi; a launcher that does not set HOME
    // still finds the cabinet's file.
    static const char PI_RC[] = "/home/pi/.advance/advmame.rc";
    FILE *f = fopen(PI_RC, "r");
    if (f) {
        fclose(f);
        vp_strlcpy(path, PI_RC, path_size);
        return VP_OK;
    }
#endif

    path[0] = '\0';
    return vp_fail(VP_ERR_NOTFOUND, "no advmame.rc found (set VP_ADVMAME_RC to choose one)");
}

int vp_rc_load(const char *path, vp_rc **out)
{
    if (!out)
        return vp_fail(VP_ERR_INVALID, "vp_rc_load: NULL argument");
    *out = nullptr;

    char found[1024];
    if (!path) {
        int r = vp_rc_default_path(found, sizeof(found));
        if (r != VP_OK) return r;
        path = found;
    }

    FILE *f = fopen(path, "r");
    if (!f)
        return vp_fail(VP_ERR_NOTFOUND, "cannot open %s", path);

    vp_rc *rc = new (std::nothrow) vp_rc;
    if (!rc) {
        fclose(f);
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }
    rc->path = path;

    std::string line;
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        line += buf;
        if (line.empty() || line.back() != '\n') {
            if (!feof(f)) continue;     // a line longer than buf: keep reading it
        }
        std::string t = trim(line);
        line.clear();
        if (t.empty() || t[0] == '#') continue;

        size_t sp = t.find_first_of(" \t");
        std::string key = t.substr(0, sp);
        std::string value = (sp == std::string::npos) ? "" : trim(t.substr(sp));

        bool replaced = false;
        for (Entry &e : rc->entries) {
            if (e.key == key) { e.value = value; replaced = true; break; }
        }
        if (!replaced) rc->entries.push_back(Entry{key, value});

        if (key.compare(0, 10, "input_map[") == 0 && key.back() == ']') {
            std::string name = key.substr(10, key.size() - 11);
            std::vector<vp_binding> bindings;
            if (!value.empty() && value != "auto") bindings = parseBindings(value);

            size_t i = 0;
            for (; i < rc->actions.size(); i++)
                if (rc->actions[i].name == name) break;
            if (bindings.empty()) {
                if (i < rc->actions.size()) rc->actions.erase(rc->actions.begin() + (long)i);
            } else if (i < rc->actions.size()) {
                rc->actions[i].bindings = bindings;
            } else {
                rc->actions.push_back(Action{name, bindings});
            }
        }
    }
    fclose(f);

    *out = rc;
    return VP_OK;
}

void vp_rc_free(vp_rc *rc)
{
    delete rc;
}

const char *vp_rc_path(const vp_rc *rc)
{
    return rc->path.c_str();
}

const char *vp_rc_get(const vp_rc *rc, const char *key)
{
    if (!rc || !key) return nullptr;
    for (const Entry &e : rc->entries)
        if (e.key == key) return e.value.c_str();
    return nullptr;
}

int vp_rc_get_bool(const vp_rc *rc, const char *key, int def)
{
    const char *v = vp_rc_get(rc, key);
    if (!v) return def;
    if (!strcmp(v, "yes") || !strcmp(v, "true") || !strcmp(v, "1")) return 1;
    if (!strcmp(v, "no") || !strcmp(v, "false") || !strcmp(v, "0")) return 0;
    return def;
}

int vp_rc_get_int(const vp_rc *rc, const char *key, int def)
{
    const char *v = vp_rc_get(rc, key);
    if (!v || !*v) return def;
    char *end = nullptr;
    long n = strtol(v, &end, 10);
    return (end && *end == '\0') ? (int)n : def;
}

int vp_rc_action_count(const vp_rc *rc)
{
    return rc ? (int)rc->actions.size() : 0;
}

const char *vp_rc_action_name(const vp_rc *rc, int i)
{
    if (!rc || i < 0 || i >= (int)rc->actions.size()) return nullptr;
    return rc->actions[(size_t)i].name.c_str();
}

static const Action *findAction(const vp_rc *rc, const char *action)
{
    if (!rc || !action) return nullptr;
    for (const Action &a : rc->actions)
        if (a.name == action) return &a;
    return nullptr;
}

int vp_rc_binding_count(const vp_rc *rc, const char *action)
{
    const Action *a = findAction(rc, action);
    return a ? (int)a->bindings.size() : 0;
}

int vp_rc_binding(const vp_rc *rc, const char *action, int i, vp_binding *out)
{
    const Action *a = findAction(rc, action);
    if (!a || !out || i < 0 || i >= (int)a->bindings.size())
        return vp_fail(VP_ERR_NOTFOUND, "no binding %d for %s", i, action ? action : "(null)");
    *out = a->bindings[(size_t)i];
    return VP_OK;
}
