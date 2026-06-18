#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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
MC_DEFINE_VECTOR(WindowRefList, MC_WindowRef*);

static MC_WM *input = NULL;
static bool input_loop = false;
static WM_KeyCombo suppressions[SUPPRESS_MAX];
static int suppression_count = 0;
static Keybind keybinds[KEYBIND_MAX];
static int keybind_count = 0;
static bool suppress_down[MC_KEY_MAX];
static bool keybind_down[MC_KEY_MAX];
static bool window_watch = false;
static bool window_poll = false;
static IdentityList *known_windows = NULL;
static uint64_t focused_window = 0;
static WindowRefList *live_windows = NULL;

static MC_WMEventType uniwm_event_offset = MC_WME_NONE;
static unsigned window_poll_tick = 0;

static bool combo_eq(const WM_KeyCombo *a, const WM_KeyCombo *b) {
    if (a->count != b->count) {
        return false;
    }

    for (size_t i = 0; i < a->count; i++) {
        if (a->keys[i] != b->keys[i]) {
            return false;
        }
    }

    return true;
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
        suppress_down[key] = down;
    }

    for (int i = 0; i < suppression_count; i++) {
        if (combo_triggered(key, &suppressions[i], suppress_down)) {
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
    if (input != NULL) {
        return WM_ERROR_OK;
    }

    if (mc_wm_init(&input, mc_win32_wm_vtab) != MCE_OK) {
        input = NULL;
        return WM_ERROR_UNKNOWN;
    }

    mc_wm_win32_set_keyboard_suppress(mc_wm_get_target(mc_wm_get_ref(input)), suppress_cb);

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
    mc_wm_register_event_group(mc_wm_get_ref(input), &uniwm_group, &uniwm_event_offset);

    return WM_ERROR_OK;
}

MC_WM *wm_input(void) {
    return input;
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

    mc_wm_request_events(mc_wm_get_ref(input), MC_WM_EVENTS_GLOBAL_KEYBOARD);
    input_loop = true;

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

    if (suppression_count >= SUPPRESS_MAX) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    suppressions[suppression_count++] = *combo;

    return WM_ERROR_OK;
}

WM_Error wm_unsuppress_key(WM *wm, const WM_KeyCombo *combo) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    for (int j = 0; j < suppression_count;) {
        if (combo_eq(&suppressions[j], combo)) {
            suppressions[j] = suppressions[--suppression_count];
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

    if (keybind_count >= KEYBIND_MAX) {
        return WM_ERROR_OUT_OF_MEMORY;
    }
    keybinds[keybind_count++] = (Keybind){ .combo = *combo, .cb = cb, .ctx = ctx };

    return WM_ERROR_OK;
}

WM_Error wm_unbind_key(WM *wm, const WM_KeyCombo *combo, void **out_ctx) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (out_ctx) {
        *out_ctx = NULL;
    }

    for (int j = 0; j < keybind_count; j++) {
        if (combo_eq(&keybinds[j].combo, combo)) {
            if (out_ctx) {
                *out_ctx = keybinds[j].ctx;
            }
            keybinds[j] = keybinds[--keybind_count];
            break;
        }
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

static void known_add(uint64_t identity) {
    IdentityList *grown = MC_VECTOR_PUSHN(known_windows, 1, (&identity));
    if (grown != NULL) {
        known_windows = grown;
    }
}

static bool known_forget(uint64_t identity) {
    size_t count = MC_VECTOR_SIZE(known_windows);
    for (size_t i = 0; i < count; i++) {
        if (MC_VECTOR_DATA(known_windows)[i] == identity) {
            MC_VECTOR_ERASE(known_windows, i, 1);
            return true;
        }
    }

    return false;
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
    if (input == NULL || type == MC_WME_NONE) {
        return;
    }

    MC_WMRef *ref = mc_wm_get_ref(input);
    MC_WMEvent event;
    if (mc_wm_event(ref, type, &event) != MCE_OK) {
        return;
    }

    mc_json_set_object((MC_Json*)event.as.raw);
    mc_json_object_add_u64((MC_Json*)event.as.raw, identity, "window");

    mc_wm_push_event(ref, &event);

    MC_WindowRef *window = NULL;
    if (wm_resolve_window(identity, &window) == WM_ERROR_OK) {
        WindowRefList *grown = MC_VECTOR_PUSHN(live_windows, 1, (&window));
        if (grown != NULL) {
            live_windows = grown;
        } else {
            mc_wm_window_unref(window);
        }
    }
}

WM_Error wm_resolve_window(uint64_t identity, MC_WindowRef **out) {
    if (out == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    if (wm_input_ensure() != WM_ERROR_OK) {
        return WM_ERROR_UNKNOWN;
    }

    MC_WMRef *ref = mc_wm_get_ref(input);

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
    if (mc_wm_window_get_identity(window, &identity) == MCE_OK) {
        IdentityList *grown = MC_VECTOR_PUSHN(poll->seen, 1, (&identity));
        if (grown != NULL) {
            poll->seen = grown;
        }

        if (poll->fire && !identity_in(known_windows, identity)) {
            emit_window_event(identity, WM_WINDOW_CREATED);
        }
    }

    mc_wm_window_unref(window);
    return MCE_OK;
}

static void poll_windows(bool fire) {
    if (input == NULL) {
        return;
    }

    WindowPoll poll = { .seen = NULL, .fire = fire };
    mc_wm_get_all_windows(mc_wm_get_ref(input), window_visit, &poll);

    if (fire) {
        uint64_t *it;
        MC_VECTOR_EACH(known_windows, it) {
            if (!identity_in(poll.seen, *it)) {
                emit_window_event(*it, WM_WINDOW_DESTROYED);
            }
        }
    }

    MC_VECTOR_FREE(known_windows);
    known_windows = poll.seen;
}

static void target_window_sink(void *ctx, uint64_t handle, WM_WindowChange change) {
    (void)ctx;

    if (input == NULL) {
        return;
    }

    if (change == WM_WINDOW_FOCUSED) {
        if (handle == focused_window) {
            return;
        }
        focused_window = handle;

        emit_window_event(handle, change);
        return;
    }

    if (change == WM_WINDOW_CREATED) {
        if (identity_in(known_windows, handle)) {
            return;
        }
        known_add(handle);
    } else {
        if (!known_forget(handle)) {
            return;
        }
    }

    emit_window_event(handle, change);
}

WM_Error wm_window_watch_ensure(void) {
    if (window_watch) {
        return WM_ERROR_OK;
    }

    WM_Error e = wm_input_ensure();
    if (e != WM_ERROR_OK) {
        return e;
    }

    window_watch = true;
    poll_windows(false);

    WM *wm = wm_process();
    bool reactive = wm != NULL
        && (wm->target_interface->capabilities & WM_TARGET_CAP_WINDOW_EVENTS)
        && wm->target_interface->on_window_changed != NULL
        && wm->target_interface->on_window_changed(wm->target, target_window_sink, NULL) == WM_ERROR_OK;

    if (!reactive) {
        window_poll = true;
    }

    return WM_ERROR_OK;
}

bool wm_should_run(void) {
    return input_loop || window_watch;
}

void wm_process_event(const MC_WMEvent *event) {
    if (event == NULL) {
        return;
    }

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

    bool was_down = key < MC_KEY_MAX && keybind_down[key];
    if (key < MC_KEY_MAX) {
        keybind_down[key] = down;
    }

    if (!down || was_down) {
        return;
    }

    size_t best = 0;
    for (int i = 0; i < keybind_count; i++) {
        if (combo_triggered(key, &keybinds[i].combo, keybind_down) && keybinds[i].combo.count > best) {
            best = keybinds[i].combo.count;
        }
    }

    for (int i = 0; i < keybind_count; i++) {
        if (keybinds[i].combo.count == best && combo_triggered(key, &keybinds[i].combo, keybind_down)) {
            keybinds[i].cb(keybinds[i].ctx);
        }
    }
}

void wm_tick(void) {
    if (live_windows != NULL) {
        MC_WindowRef **it;
        MC_VECTOR_EACH(live_windows, it) {
            mc_wm_window_unref(*it);
        }
        MC_VECTOR_FREE(live_windows);
        live_windows = NULL;
    }

    if (window_poll && window_poll_tick % WINDOW_POLL_TICKS == 0) {
        poll_windows(true);
    }
    window_poll_tick++;
}
