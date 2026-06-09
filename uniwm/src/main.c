#include <stdio.h>
#include <string.h>

#include <mc/data/alloc.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <uniwm/wm.h>
#include <uniwm-windows/target.h>
#include <uniwm-lua/uniwm-lua.h>

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

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *script = parse_script(argc, argv);

    wm_set_default(uniwm_windows_target, &mc_alloc_malloc);

    lua_State *L = luaL_newstate();
    if (L == NULL) {
        fprintf(stderr, "uniwm: failed to initialise lua\n");
        return 1;
    }

    luaL_openlibs(L);
    uniwm_lua_open(L);

    int rc = run_script(L, script);
    if (rc == 0) {
        wm_run();
    }

    lua_close(L);
    wm_process_fini();

    return rc == 0 ? 0 : 1;
}
