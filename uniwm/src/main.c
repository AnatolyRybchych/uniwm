#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <mc/time.h>
#include <mc/wm/wm.h>
#include <mc/wm/event.h>

#include <uniwm/wm.h>
#include <uniwm-windows/target.h>
#include <uniwm-lua/uniwm-lua.h>

#include "proc.h"

#define SUPERVISOR_MIN_HEALTHY_MS 2000
#define SUPERVISOR_RESTART_DELAY_MS 1000
#define CHILD_ARGS_MAX 64

static int run_script(lua_State *L, const char *path) {
    if (luaL_loadfile(L, path) != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK) {
        fprintf(stderr, "uniwm: %s: %s\n", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

static const char *parse_script(int argc, char **argv) {
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            return argv[i + 1];
        }
    }

    return "config.lua";
}

static int run_worker(int argc, char **argv) {
    const char *script = parse_script(argc, argv);

    wm_set_default(uniwm_windows_target);

    lua_State *L = luaL_newstate();
    if (L == NULL) {
        fprintf(stderr, "uniwm: failed to initialise lua\n");
        return 1;
    }

    luaL_openlibs(L);
    uniwm_lua_open(L);

    int rc = run_script(L, script);
    if (rc == 0 && wm_should_run()) {
        MC_WM *input = wm_input();
        MC_WMRef *ref = mc_wm_get_ref(input);

        for (;;) {
            MC_WMEvent event;
            while (mc_wm_poll_event(input, &event) == MCE_OK) {
                mc_wm_dispatch_event_callbacks(ref, &event);
                wm_process_event(&event);
            }

            wm_tick();
            mc_sleep(&(MC_Time){ .nsec = 16000000 });
        }
    }

    lua_close(L);
    wm_process_fini();

    return rc == 0 ? 0 : 1;
}

static int run_supervisor(bool restart, const char *const *args, size_t count) {
    for (;;) {
        uint64_t start = wm_proc_now_ms();

        WM_Child *child = NULL;
        if (wm_proc_spawn_self(args, count, &child) != WM_ERROR_OK) {
            fprintf(stderr, "uniwm: failed to start child process\n");
            if (!restart) {
                return 1;
            }
            wm_proc_sleep_ms(SUPERVISOR_RESTART_DELAY_MS);
            continue;
        }

        int code = 0;
        wm_proc_wait(child, &code);
        wm_proc_close(child);

        if (!restart) {
            return code;
        }

        uint64_t ran = wm_proc_now_ms() - start;
        fprintf(stderr, "uniwm: child exited (code %d) after %llums; restarting\n", code, (unsigned long long)ran);

        if (ran < SUPERVISOR_MIN_HEALTHY_MS) {
            wm_proc_sleep_ms(SUPERVISOR_RESTART_DELAY_MS);
        }
    }
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *child_args[CHILD_ARGS_MAX];
    size_t child_count = 0;
    child_args[child_count++] = "--worker";

    bool worker = false;
    bool restart = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--worker") == 0) {
            worker = true;
            continue;
        }

        if (strcmp(argv[i], "--restart") == 0) {
            restart = true;
            continue;
        }

        if (child_count < CHILD_ARGS_MAX) {
            child_args[child_count++] = argv[i];
        }
    }

    if (worker) {
        return run_worker(argc, argv);
    }

    return run_supervisor(restart, child_args, child_count);
}
