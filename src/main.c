#include <stdio.h>
#include <stdbool.h>

#include <windows.h>

#include <mc/data/alloc.h>
#include <mc/data/str.h>
#include <mc/time.h>
#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/win32_wm/wm.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>
#include <uniwm-windows/target.h>

static WM desktops;
static bool running = true;
static bool win_down = false;
static int pending_switch = -1;

static bool suppress(MC_TargetWM *wm, int vk, bool down) {
    (void)wm;

    if (vk == VK_LWIN || vk == VK_RWIN) {
        win_down = down;
        return true;
    }

    if (!down || !win_down) {
        return false;
    }

    if (vk >= '1' && vk <= '9') {
        pending_switch = vk - '1';
        return true;
    }

    if (vk == 'Q') {
        running = false;
        return true;
    }

    return false;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (wm_init(&desktops, uniwm_windows_target, &mc_alloc_malloc) != WM_ERROR_OK) {
        fprintf(stderr, "uniwm: failed to initialise desktop target\n");
        return 1;
    }

    MC_WM *input;
    if (mc_wm_init(&input, mc_win32_wm_vtab) != MCE_OK) {
        fprintf(stderr, "uniwm: failed to initialise input\n");
        wm_fini(&desktops);
        return 1;
    }

    mc_wm_request_events(input, MC_WM_EVENTS_GLOBAL_KEYBOARD);
    mc_wm_win32_set_keyboard_suppress(mc_wm_get_target(input), suppress);

    puts("uniwm: WIN+<1-9> switch desktop, WIN+Q quit");

    while (running) {
        MC_WMEvent event;
        while (mc_wm_poll_event(input, &event) == MCE_OK) {
        }

        if (pending_switch >= 0) {
            WM_VDesktopSpan span = wm_vdesktops(&desktops);
            if ((size_t)pending_switch < span.count) {
                WM_VDesktop *target = span.desktops[pending_switch];
                wm_vdesktop_switch(&desktops, target);

                MC_Str name = wm_vdesktop_name(&desktops, target);
                printf("switched to desktop %d: %.*s\n", pending_switch + 1, (int)MC_STR_LEN(name), name.beg);
            }

            pending_switch = -1;
        }

        mc_sleep(&(MC_Time){.nsec = 16000000});
    }

    mc_wm_destroy(input);
    wm_fini(&desktops);

    return 0;
}
