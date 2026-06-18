#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <mc/time.h>
#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/key.h>
#include <mc/wm/resolver.h>
#include <mc/win32_wm/wm.h>
#include <mc/data/vector.h>
#include <mc/data/json.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>

#define SUPPRESS_MAX 128
#define KEYBIND_MAX 128
#define WINDOW_POLL_TICKS 8

typedef struct Keybind {
    WM_KeyCombo combo;
    void (*cb)(void *ctx);
    void *ctx;
} Keybind;

MC_DEFINE_VECTOR(IdentityList, uint64_t);

typedef struct RegisteredWindow {
    uint64_t identity;
    MC_WindowRef *window;
    bool known;
} RegisteredWindow;

MC_DEFINE_VECTOR(WindowList, RegisteredWindow);

static struct {
    MC_WM *wm;
    bool loop;
} input_wm;

static struct {
    WM_KeyCombo combos[SUPPRESS_MAX];
    int count;
    bool down[MC_KEY_MAX];
} suppress;

static struct {
    Keybind binds[KEYBIND_MAX];
    int count;
    bool down[MC_KEY_MAX];
} keybind;

static struct {
    bool active;
    bool poll;
    WindowList *list;
    uint64_t focused;
} watch;

static MC_WMEventType uniwm_event_offset = MC_WME_NONE;
static unsigned window_poll_tick = 0;

static bool combo_eq(const WM_KeyCombo *a, const WM_KeyCombo *b) {
    return a->count == b->count && memcmp(a->keys, b->keys, sizeof(MC_Key[a->count])) == 0;
}

static bool combo_triggered(MC_Key key, const WM_KeyCombo *c, const bool *down) {
    if (c->count == 0 || key != c->keys[c->count - 1]) {
        return false;
    }

    for (size_t m = 0; m + 1 < c->count; m++) {
        MC_Key mod = c->keys[m];
        if (mod >= MC_KEY_MAX || !down[mod]) {
            return false;
        }
    }

    return true;
}

static bool suppress_cb(MC_TargetWM *tw, MC_Key key, bool down) {
    (void)tw;

    if (key < MC_KEY_MAX) {
        suppress.down[key] = down;
    }

    for (int i = 0; i < suppress.count; i++) {
        if (combo_triggered(key, &suppress.combos[i], suppress.down)) {
            return true;
        }
    }

    return false;
}

static MC_Error uniwm_event_to_json(MC_Alloc *alloc, const MC_WMEvent *event, MC_Json *out) {
    (void)alloc;

    MC_Json *raw = (MC_Json*)event->as.raw;
    size_t n = mc_json_length(raw);
    for (size_t i = 0; i < n; i++) {
        MC_Str key;
        MC_Json *value;
        if (mc_json_object_at(raw, i, &key, &value) != MCE_OK) {
            continue;
        }

        MC_Json *member;
        MC_Error e = mc_json_object_add_new(out, &member, "%.*s", (int)MC_STR_LEN(key), key.beg);
        if (e != MCE_OK) {
            return e;
        }

        e = mc_json_copy(member, value);
        if (e != MCE_OK) {
            return e;
        }
    }

    return MCE_OK;
}

WM_Error wm_input_ensure(void) {
    if (input_wm.wm != NULL) {
        return WM_ERROR_OK;
    }

    if (mc_wm_init(&input_wm.wm, mc_win32_wm_vtab) != MCE_OK) {
        input_wm.wm = NULL;
        return WM_ERROR_UNKNOWN;
    }

    mc_wm_win32_set_keyboard_suppress(mc_wm_get_target(mc_wm_get_ref(input_wm.wm)), suppress_cb);

    static const MC_WMEventDefinition uniwm_events[WM_UNIWM_EVENT_COUNT] = {
        [WM_UNIWM_VDESKTOP_CHANGED] = { .name = "VDESKTOP_CHANGED" },
        [WM_UNIWM_WINDOW_CREATED] = { .name = "WINDOW_CREATED" },
        [WM_UNIWM_WINDOW_DESTROYED] = { .name = "WINDOW_DESTROYED" },
        [WM_UNIWM_WINDOW_FOCUSED] = { .name = "WINDOW_FOCUSED" },
    };
    MC_WMEventGroupDef uniwm_group = {
        .name = "UNIWM",
        .events = uniwm_events,
        .size = WM_UNIWM_EVENT_COUNT,
        .reserve = 0,
        .to_json = uniwm_event_to_json,
        .from_json = NULL,
    };
    mc_wm_register_event_group(mc_wm_get_ref(input_wm.wm), &uniwm_group, &uniwm_event_offset);

    return WM_ERROR_OK;
}

MC_WM *wm_input(void) {
    return input_wm.wm;
}

MC_WMEventType wm_uniwm_event_type(WM_UniwmEvent which) {
    if (uniwm_event_offset == MC_WME_NONE || which >= WM_UNIWM_EVENT_COUNT) {
        return MC_WME_NONE;
    }

    return (MC_WMEventType)(uniwm_event_offset + which);
}

static WM_Error enable_keyboard(void) {
    WM_Error e = wm_input_ensure();
    if (e != WM_ERROR_OK) {
        return e;
    }

    mc_wm_request_events(mc_wm_get_ref(input_wm.wm), MC_WM_EVENTS_GLOBAL_KEYBOARD);
    input_wm.loop = true;

    return WM_ERROR_OK;
}

WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    WM_Error e = enable_keyboard();
    if (e != WM_ERROR_OK) {
        return e;
    }

    if (suppress.count >= SUPPRESS_MAX) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    suppress.combos[suppress.count++] = *combo;

    return WM_ERROR_OK;
}

WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    for (int j = 0; j < suppress.count;) {
        if (combo_eq(&suppress.combos[j], combo)) {
            suppress.combos[j] = suppress.combos[--suppress.count];
        } else {
            j++;
        }
    }

    return WM_ERROR_OK;
}

WM_Error wm_bind_key(WM *wm, const WM_KeyCombo *combo, void (*cb)(void *ctx), void *ctx) {
    (void)wm;

    if (combo == NULL || cb == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    WM_Error e = enable_keyboard();
    if (e != WM_ERROR_OK) {
        return e;
    }

    if (keybind.count >= KEYBIND_MAX) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    keybind.binds[keybind.count++] = (Keybind){ .combo = *combo, .cb = cb, .ctx = ctx };

    return WM_ERROR_OK;
}

WM_Error wm_unbind_key(WM *wm, const WM_KeyCombo *combo, void **out_ctx) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    void *ctx_found = NULL;
    for (int j = 0; j < keybind.count; j++) {
        if (combo_eq(&keybind.binds[j].combo, combo)) {
            ctx_found = keybind.binds[j].ctx;
            keybind.binds[j] = keybind.binds[--keybind.count];
            break;
        }
    }
    
    if (out_ctx) {
        *out_ctx = ctx_found;
    }

    return WM_ERROR_OK;
}

static bool identity_in(const IdentityList *list, uint64_t identity) {
    if (list == NULL) {
        return false;
    }

    size_t count = MC_VECTOR_SIZE(list);
    for (size_t i = 0; i < count; i++) {
        if (MC_VECTOR_DATA(list)[i] == identity) {
            return true;
        }
    }

    return false;
}

static RegisteredWindow *find_window(uint64_t identity) {
    size_t count = MC_VECTOR_SIZE(watch.list);
    for (size_t i = 0; i < count; i++) {
        if (MC_VECTOR_DATA(watch.list)[i].identity == identity) {
            return &MC_VECTOR_DATA(watch.list)[i];
        }
    }

    return NULL;
}

static void register_window_ref(uint64_t identity, MC_WindowRef *window, bool known) {
    RegisteredWindow entry = { .identity = identity, .window = window, .known = known };
    WindowList *grown = MC_VECTOR_PUSHN(watch.list, 1, (&entry));
    if (grown != NULL) {
        watch.list = grown;
    } else {
        mc_wm_window_unref(window);
    }
}

static void register_window(uint64_t identity, bool known) {
    if (find_window(identity) != NULL) {
        return;
    }

    MC_WindowRef *window = NULL;
    if (wm_resolve_window(identity, &window) == WM_ERROR_OK) {
        register_window_ref(identity, window, known);
    }
}

static void unregister_window(uint64_t identity) {
    size_t count = MC_VECTOR_SIZE(watch.list);
    for (size_t i = 0; i < count; i++) {
        if (MC_VECTOR_DATA(watch.list)[i].identity == identity) {
            mc_wm_window_unref(MC_VECTOR_DATA(watch.list)[i].window);
            MC_VECTOR_ERASE(watch.list, i, 1);
            return;
        }
    }
}

static WM_UniwmEvent uniwm_event_of(WM_WindowChange change) {
    switch (change) {
    case WM_WINDOW_CREATED:
        return WM_UNIWM_WINDOW_CREATED;
    case WM_WINDOW_DESTROYED:
        return WM_UNIWM_WINDOW_DESTROYED;
    case WM_WINDOW_FOCUSED:
        return WM_UNIWM_WINDOW_FOCUSED;
    default:
        return WM_UNIWM_EVENT_COUNT;
    }
}

static void emit_window_event(uint64_t identity, WM_WindowChange change) {
    MC_WMEventType type = wm_uniwm_event_type(uniwm_event_of(change));
    if (input_wm.wm == NULL || type == MC_WME_NONE) {
        return;
    }

    MC_WMRef *ref = mc_wm_get_ref(input_wm.wm);
    MC_WMEvent event;
    if (mc_wm_event(ref, type, &event) != MCE_OK) {
        return;
    }

    mc_json_set_object((MC_Json*)event.as.raw);
    mc_json_object_add_u64((MC_Json*)event.as.raw, identity, "window_id");

    MC_Json *window = NULL;
    if (mc_json_object_add_new((MC_Json*)event.as.raw, &window, "window") == MCE_OK) {
        mc_json_set_null(window);
    }

    mc_wm_push_event(ref, &event);
}

WM_Error wm_resolve_window(uint64_t identity, MC_WindowRef **out) {
    if (out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (wm_input_ensure() != WM_ERROR_OK) {
        return WM_ERROR_UNKNOWN;
    }

    MC_WMRef *ref = mc_wm_get_ref(input_wm.wm);

    uint64_t resolved;
    if (mc_wm_win32_identity_from_hwnd(mc_wm_get_target(ref), (HWND)(uintptr_t)identity, &resolved) != MCE_OK) {
        return WM_ERROR_UNKNOWN;
    }

    if (mc_wm_resolve_window(ref, resolved, out) != MCE_OK) {
        return WM_ERROR_UNKNOWN;
    }

    return WM_ERROR_OK;
}

typedef struct WindowPoll {
    IdentityList *seen;
    bool fire;
} WindowPoll;

static MC_Error window_visit(MC_WindowRef *window, void *ctx) {
    WindowPoll *poll = ctx;

    uint64_t identity;
    if (mc_wm_window_get_identity(window, &identity) != MCE_OK) {
        mc_wm_window_unref(window);
        return MCE_OK;
    }

    IdentityList *grown = MC_VECTOR_PUSHN(poll->seen, 1, (&identity));
    if (grown != NULL) {
        poll->seen = grown;
    }

    if (find_window(identity) != NULL) {
        mc_wm_window_unref(window);
        return MCE_OK;
    }

    register_window_ref(identity, window, true);
    if (poll->fire) {
        emit_window_event(identity, WM_WINDOW_CREATED);
    }

    return MCE_OK;
}

static void poll_windows(bool fire) {
    if (input_wm.wm == NULL) {
        return;
    }

    WindowPoll poll = { .seen = NULL, .fire = fire };
    mc_wm_get_all_windows(mc_wm_get_ref(input_wm.wm), window_visit, &poll);

    if (fire) {
        size_t count = MC_VECTOR_SIZE(watch.list);
        for (size_t i = 0; i < count; i++) {
            uint64_t identity = MC_VECTOR_DATA(watch.list)[i].identity;
            if (!identity_in(poll.seen, identity)) {
                emit_window_event(identity, WM_WINDOW_DESTROYED);
            }
        }
    }

    MC_VECTOR_FREE(poll.seen);
}

static void target_window_sink(void *ctx, uint64_t handle, WM_WindowChange change) {
    (void)ctx;

    if (input_wm.wm == NULL) {
        return;
    }

    switch (change) {
    case WM_WINDOW_OPENED:
        register_window(handle, false);
        return;

    case WM_WINDOW_CREATED: {
        RegisteredWindow *w = find_window(handle);
        if (w == NULL) {
            register_window(handle, true);
        } else if (!w->known) {
            w->known = true;
        } else {
            return;
        }

        emit_window_event(handle, WM_WINDOW_CREATED);
        return;
    }

    case WM_WINDOW_DESTROYED: {
        RegisteredWindow *w = find_window(handle);
        if (w == NULL) {
            return;
        }

        if (w->known) {
            emit_window_event(handle, WM_WINDOW_DESTROYED);
        } else {
            unregister_window(handle);
        }
        return;
    }

    case WM_WINDOW_FOCUSED:
        if (handle == watch.focused) {
            return;
        }
        watch.focused = handle;

        register_window(handle, false);
        emit_window_event(handle, WM_WINDOW_FOCUSED);
        return;

    default:
        return;
    }
}

WM_Error wm_window_watch_ensure(void) {
    if (watch.active) {
        return WM_ERROR_OK;
    }

    WM_Error e = wm_input_ensure();
    if (e != WM_ERROR_OK) {
        return e;
    }

    watch.active = true;
    poll_windows(false);

    WM *wm = wm_process();
    bool reactive = wm != NULL
        && (wm->target_interface->capabilities & WM_TARGET_CAP_WINDOW_EVENTS)
        && wm->target_interface->on_window_changed != NULL
        && wm->target_interface->on_window_changed(wm->target, target_window_sink, NULL) == WM_ERROR_OK;

    if (!reactive) {
        watch.poll = true;
    }

    return WM_ERROR_OK;
}

bool wm_should_run(void) {
    return input_wm.loop || watch.active;
}

static void process_keybinds(const MC_WMEvent *event) {
    MC_Key key;
    bool down;
    if (event->type == MC_WME_GLOBAL_KEY_DOWN) {
        key = event->as.global_key_down.key;
        down = true;
    } else if (event->type == MC_WME_GLOBAL_KEY_UP) {
        key = event->as.global_key_up.key;
        down = false;
    } else {
        return;
    }

    bool was_down = key < MC_KEY_MAX && keybind.down[key];
    if (key < MC_KEY_MAX) {
        keybind.down[key] = down;
    }

    if (!down || was_down) {
        return;
    }

    size_t best = 0;
    for (int i = 0; i < keybind.count; i++) {
        if (combo_triggered(key, &keybind.binds[i].combo, keybind.down) && keybind.binds[i].combo.count > best) {
            best = keybind.binds[i].combo.count;
        }
    }

    for (int i = 0; i < keybind.count; i++) {
        if (keybind.binds[i].combo.count == best && combo_triggered(key, &keybind.binds[i].combo, keybind.down)) {
            keybind.binds[i].cb(keybind.binds[i].ctx);
        }
    }
}

static void release_destroyed_window(const MC_WMEvent *event) {
    if (event->type != wm_uniwm_event_type(WM_UNIWM_WINDOW_DESTROYED)) {
        return;
    }

    uint64_t identity = mc_json_object_as_u64((MC_Json*)event->as.raw, "window");
    unregister_window(identity);
}

void wm_process_event(const MC_WMEvent *event) {
    if (event == NULL) {
        return;
    }

    process_keybinds(event);

    release_destroyed_window(event);
}

void wm_tick(void) {
    if (watch.poll && window_poll_tick % WINDOW_POLL_TICKS == 0) {
        poll_windows(true);
    }
    window_poll_tick++;
}
