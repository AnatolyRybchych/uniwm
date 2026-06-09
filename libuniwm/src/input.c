#include <stddef.h>
#include <stdbool.h>

#include <mc/time.h>
#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/key.h>
#include <mc/win32_wm/wm.h>

#include <uniwm/wm.h>

#define SUPPRESS_MAX 128
#define KEYBIND_MAX 128

typedef struct Keybind {
    WM_KeyCombo combo;
    void (*cb)(void *ctx);
    void *ctx;
} Keybind;

static MC_WM *input = NULL;
static WM_KeyCombo suppressions[SUPPRESS_MAX];
static int suppression_count = 0;
static Keybind keybinds[KEYBIND_MAX];
static int keybind_count = 0;
static bool suppress_down[MC_KEY_MAX];
static bool keybind_down[MC_KEY_MAX];

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

static WM_Error ensure_input(void) {
    if (input != NULL) {
        return WM_ERROR_OK;
    }

    if (mc_wm_init(&input, mc_win32_wm_vtab) != MCE_OK) {
        input = NULL;
        return WM_ERROR_UNKNOWN;
    }

    mc_wm_request_events(input, MC_WM_EVENTS_GLOBAL_KEYBOARD);
    mc_wm_win32_set_keyboard_suppress(mc_wm_get_target(input), suppress_cb);

    return WM_ERROR_OK;
}

WM_Error wm_suppress_key(WM *wm, const WM_KeyCombo *combo) {
    (void)wm;

    if (combo == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    WM_Error e = ensure_input();
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

    WM_Error e = ensure_input();
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

WM_Error wm_run(void) {
    if (input == NULL) {
        return WM_ERROR_OK;
    }

    for (;;) {
        MC_WMEvent event;
        while (mc_wm_poll_event(input, &event) == MCE_OK) {
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
                for (int i = 0; i < keybind_count; i++) {
                    if (combo_triggered(key, &keybinds[i].combo, keybind_down)) {
                        keybinds[i].cb(keybinds[i].ctx);
                    }
                }
            }
        }

        mc_sleep(&(MC_Time){.nsec = 16000000});
    }
}
