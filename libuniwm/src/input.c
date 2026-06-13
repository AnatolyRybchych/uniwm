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

#include <uniwm/wm.h>

#define SUPPRESS_MAX 128
#define KEYBIND_MAX 128
#define WINDOW_POLL_TICKS 8

typedef struct Keybind {
    WM_KeyCombo combo;
    void (*cb)(void *ctx);
    void *ctx;
} Keybind;

MC_DEFINE_VECTOR(IdentityList, uint64_t);

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

WM_Error wm_input_ensure(void) {
    if (input != NULL) {
        return WM_ERROR_OK;
    }

    if (mc_wm_init(&input, mc_win32_wm_vtab) != MCE_OK) {
        input = NULL;
        return WM_ERROR_UNKNOWN;
    }

    mc_wm_win32_set_keyboard_suppress(mc_wm_get_target(mc_wm_get_ref(input)), suppress_cb);

    return WM_ERROR_OK;
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

static void dispatch_window(MC_WindowRef *window, WM_WindowChange change) {
    WM_Event event = {
        .type = change == WM_WINDOW_CREATED ? WM_EVENT_WINDOW_CREATED : WM_EVENT_WINDOW_DESTROYED,
    };
    if (change == WM_WINDOW_CREATED) {
        event.as.window_created.window = window;
    } else {
        event.as.window_destroyed.window = window;
    }

    wm_dispatch_event(wm_process(), &event);
}

static void notify_change(uint64_t identity, WM_WindowChange change) {
    MC_WindowRef *window;
    if (mc_wm_resolve_window(mc_wm_get_ref(input), identity, &window) != MCE_OK) {
        return;
    }

    dispatch_window(window, change);

    mc_wm_window_unref(window);
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
            dispatch_window(window, WM_WINDOW_CREATED);
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

    if (fire && wm_has_subscribers(wm_process(), WM_EVENT_WINDOW_DESTROYED)) {
        uint64_t *it;
        MC_VECTOR_EACH(known_windows, it) {
            if (!identity_in(poll.seen, *it)) {
                notify_change(*it, WM_WINDOW_DESTROYED);
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

    uint64_t identity;
    if (mc_wm_win32_identity_from_hwnd(mc_wm_get_target(mc_wm_get_ref(input)), (HWND)(uintptr_t)handle, &identity) != MCE_OK) {
        return;
    }

    notify_change(identity, change);
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

    if (wm_watch_window_changed(wm_process(), target_window_sink, NULL) != WM_ERROR_OK) {
        window_poll = true;
    }

    return WM_ERROR_OK;
}

WM_Error wm_run(void) {
    if (!input_loop && !window_watch) {
        return WM_ERROR_OK;
    }

    unsigned tick = 0;
    for (;;) {
        MC_WMEvent event;
        while (mc_wm_poll_event(input, &event) == MCE_OK) {
            mc_wm_dispatch_event_callbacks(mc_wm_get_ref(input), &event);

            MC_Key key;
            bool down;
            if (event.type == MC_WME_GLOBAL_KEY_DOWN) {
                key = event.as.global_key_down.key;
                down = true;
            } else if (event.type == MC_WME_GLOBAL_KEY_UP) {
                key = event.as.global_key_up.key;
                down = false;
            } else {
                continue;
            }

            bool was_down = key < MC_KEY_MAX && keybind_down[key];
            if (key < MC_KEY_MAX) {
                keybind_down[key] = down;
            }

            if (down && !was_down) {
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
        }

        if (window_poll && tick % WINDOW_POLL_TICKS == 0) {
            poll_windows(true);
        }
        tick++;

        mc_sleep(&(MC_Time){.nsec = 16000000});
    }
}
