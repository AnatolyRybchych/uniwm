#include <uniwm-lua/uniwm-lua.h>

#include <stdio.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#include <mc/data/str.h>
#include <mc/data/alloc.h>
#include <mc/wm-lua/wm-lua.h>

#include <uniwm/wm.h>
#include <uniwm-windows/target.h>

typedef struct LuaCallback {
    lua_State *L;
    int ref;
} LuaCallback;

static void lua_callback_free(LuaCallback *k) {
    luaL_unref(k->L, LUA_REGISTRYINDEX, k->ref);
    mc_free(wm_allocator, k);
}

static WM *current_wm(lua_State *L) {
    WM *wm = wm_process();
    if (wm == NULL) {
        luaL_error(L, "uniwm: no window manager available for this process");
    }

    return wm;
}

#define VDESK_CACHE "libuniwm.vdesktops"

static void push_vdesk_cache(lua_State *L) {
    if (luaL_getsubtable(L, LUA_REGISTRYINDEX, VDESK_CACHE)) {
        return;
    }

    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
}

static bool vdesk_cache_get(lua_State *L, const void *key) {
    push_vdesk_cache(L);
    if (lua_rawgetp(L, -1, key) == LUA_TNIL) {
        lua_pop(L, 2);
        return false;
    }

    lua_remove(L, -2);
    return true;
}

static void vdesk_cache_put(lua_State *L, const void *key) {
    push_vdesk_cache(L);
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, key);
    lua_pop(L, 1);
}

static WM_VDesktop *checked_vdesk(lua_State *L, WM *wm) {
    WM_VDesktop *d = lua_touserdata(L, lua_upvalueindex(1));
    WM_VDesktopSpan span = wm_vdesktops(wm);
    for (size_t i = 0; i < span.count; i++) {
        if (span.desktops[i] == d) {
            return d;
        }
    }

    luaL_error(L, "uniwm: virtual desktop no longer exists");
    return NULL;
}

static int vdesk_switch(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktop *d = checked_vdesk(L, wm);

    if (wm_vdesktop_switch(wm, d) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: switch failed");
    }

    return 0;
}

typedef struct CollectVDeskWindows {
    lua_State *L;
    int table_index;
    lua_Integer count;
} CollectVDeskWindows;

static void collect_vdesk_window(MC_WindowRef *window, void *ctx) {
    CollectVDeskWindows *c = ctx;

    mc_wm_lua_push_window(c->L, window);
    lua_rawseti(c->L, c->table_index, ++c->count);
}

static int vdesk_windows(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktop *d = checked_vdesk(L, wm);

    lua_newtable(L);
    CollectVDeskWindows c = {.L = L, .table_index = lua_gettop(L), .count = 0};
    if (wm_vdesktop_windows(wm, d, collect_vdesk_window, &c) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: failed to list windows on virtual desktop");
    }

    return 1;
}

static int vdesk_move_window(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktop *d = checked_vdesk(L, wm);
    MC_WindowRef *ref = mc_wm_lua_check_window(L, 2);

    uint64_t handle;
    if (mc_wm_window_get_identity(ref, &handle) != MCE_OK) {
        return luaL_error(L, "uniwm: move_window: window has no movable handle");
    }

    if (wm_vdesktop_move_window(wm, d, handle) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: move_window: failed");
    }

    return 0;
}

static int vdesk_size(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktop *d = checked_vdesk(L, wm);

    MC_Size2U size;
    if (wm_vdesktop_size(wm, d, &size) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: failed to get virtual desktop size");
    }

    lua_createtable(L, 0, 2);
    lua_pushinteger(L, size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, size.height);
    lua_setfield(L, -2, "height");
    return 1;
}

static void push_vdesk(lua_State *L, WM *wm, WM_VDesktop *d) {
    if (vdesk_cache_get(L, d)) {
        return;
    }

    lua_createtable(L, 0, 5);

    MC_Str name = wm_vdesktop_name(wm, d);
    lua_pushlstring(L, name.beg ? name.beg : "", MC_STR_LEN(name));
    lua_setfield(L, -2, "name");

    lua_pushlightuserdata(L, d);
    lua_pushcclosure(L, vdesk_switch, 1);
    lua_setfield(L, -2, "switch");

    lua_pushlightuserdata(L, d);
    lua_pushcclosure(L, vdesk_windows, 1);
    lua_setfield(L, -2, "windows");

    lua_pushlightuserdata(L, d);
    lua_pushcclosure(L, vdesk_size, 1);
    lua_setfield(L, -2, "size");

    lua_pushlightuserdata(L, d);
    lua_pushcclosure(L, vdesk_move_window, 1);
    lua_setfield(L, -2, "move_window");

    vdesk_cache_put(L, d);
}

static void push_vdesk_ptr(lua_State *L, WM *wm, WM_VDesktop *vdesk) {
    WM_VDesktopSpan span = wm_vdesktops(wm);
    for (size_t i = 0; i < span.count; i++) {
        if (span.desktops[i] == vdesk) {
            push_vdesk(L, wm, vdesk);
            return;
        }
    }

    lua_pushnil(L);
}

static int l_vdesk_list(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktopSpan span = wm_vdesktops(wm);

    lua_createtable(L, (int)span.count, 0);
    for (size_t i = 0; i < span.count; i++) {
        push_vdesk(L, wm, span.desktops[i]);
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }

    return 1;
}

static int l_vdesk_create(lua_State *L) {
    const char *name = luaL_checkstring(L, 2);
    WM *wm = current_wm(L);

    WM_VDesktop *d = NULL;
    if (wm_vdesktop_create(wm, mc_strc(name), &d) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.vdesktop:create: failed");
    }

    push_vdesk_ptr(L, wm, d);
    return 1;
}

static int l_vdesk_current(lua_State *L) {
    WM *wm = current_wm(L);

    push_vdesk_ptr(L, wm, wm_vdesktop_current(wm));
    return 1;
}

static int l_supress_key(lua_State *L) {
    const char *spec = luaL_checkstring(L, 1);

    WM_KeyCombo combo;
    if (wm_key_combo_from_str(spec, &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.supress_key: invalid key spec \"%s\"", spec);
    }

    if (wm_suppress_key(current_wm(L), &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.supress_key: failed");
    }

    return 0;
}

static int l_unsupress_key(lua_State *L) {
    const char *spec = luaL_checkstring(L, 1);

    WM_KeyCombo combo;
    if (wm_key_combo_from_str(spec, &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.unsupress_key: invalid key spec \"%s\"", spec);
    }

    if (wm_unsuppress_key(current_wm(L), &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.unsupress_key: failed");
    }

    return 0;
}

static void keybind_cb(void *ctx) {
    LuaCallback *k = ctx;

    lua_rawgeti(k->L, LUA_REGISTRYINDEX, k->ref);
    if (lua_pcall(k->L, 0, 0, 0) != LUA_OK) {
        fprintf(stderr, "uniwm: keybind callback error: %s\n", lua_tostring(k->L, -1));
        lua_pop(k->L, 1);
    }
}

static int l_register_keybind(lua_State *L) {
    const char *spec = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    WM_KeyCombo combo;
    if (wm_key_combo_from_str(spec, &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.register_keybind: invalid key spec \"%s\"", spec);
    }

    WM *wm = current_wm(L);

    void *old = NULL;
    wm_unbind_key(wm, &combo, &old);
    if (old) {
        lua_callback_free(old);
    }

    LuaCallback *k = NULL;
    if (mc_alloc(wm_allocator, sizeof(*k), (void **)&k) != MCE_OK) {
        return luaL_error(L, "uniwm.register_keybind: out of memory");
    }
    k->L = L;
    lua_pushvalue(L, 2);
    k->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if (wm_bind_key(wm, &combo, keybind_cb, k) != WM_ERROR_OK) {
        lua_callback_free(k);
        return luaL_error(L, "uniwm.register_keybind: failed");
    }

    return 0;
}

static int l_window(lua_State *L) {
    uint64_t identity = (uint64_t)luaL_checkinteger(L, 1);

    MC_WindowRef *window = NULL;
    if (wm_resolve_window(identity, &window) != WM_ERROR_OK) {
        lua_pushnil(L);
        return 1;
    }

    mc_wm_lua_push_window(L, window);
    return 1;
}

static int l_unregister_keybind(lua_State *L) {
    const char *spec = luaL_checkstring(L, 1);

    WM_KeyCombo combo;
    if (wm_key_combo_from_str(spec, &combo) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm.unregister_keybind: invalid key spec \"%s\"", spec);
    }

    void *old = NULL;
    wm_unbind_key(current_wm(L), &combo, &old);
    if (old) {
        lua_callback_free(old);
    }

    return 0;
}

static int open_libuniwm(lua_State *L) {
    static const luaL_Reg vdesk_fns[] = {
        { "list", l_vdesk_list },
        { "current", l_vdesk_current },
        { "create", l_vdesk_create },
        { NULL, NULL },
    };

    lua_createtable(L, 0, 3);

    luaL_newlib(L, vdesk_fns);
    lua_setfield(L, -2, "vdesktop");

    lua_pushcfunction(L, l_supress_key);
    lua_setfield(L, -2, "supress_key");
    lua_pushcfunction(L, l_unsupress_key);
    lua_setfield(L, -2, "unsupress_key");
    lua_pushcfunction(L, l_register_keybind);
    lua_setfield(L, -2, "register_keybind");
    lua_pushcfunction(L, l_unregister_keybind);
    lua_setfield(L, -2, "unregister_keybind");
    lua_pushcfunction(L, l_window);
    lua_setfield(L, -2, "window");

    return 1;
}

WM_Error uniwm_lua_open(lua_State *L) {
    if (L == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    luaL_requiref(L, "uniwm", open_libuniwm, 0);
    lua_pop(L, 1);

    mc_wm_lua_open(L, NULL);

    return WM_ERROR_OK;
}

int luaopen_libuniwm(lua_State *L) {
    wm_set_default(uniwm_windows_target);
    mc_wm_lua_open(L, NULL);
    return open_libuniwm(L);
}
