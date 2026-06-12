#include <uniwm-lua/uniwm-lua.h>

#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

#include <mc/data/str.h>
#include <mc/data/alloc.h>
#include <mc/wm-lua/wm-lua.h>

#include <uniwm/wm.h>

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

static int vdesk_switch(lua_State *L) {
    WM *wm = current_wm(L);
    lua_Integer index = lua_tointeger(L, lua_upvalueindex(1));
    WM_VDesktopSpan span = wm_vdesktops(wm);

    if (index < 1 || (size_t)index > span.count) {
        return luaL_error(L, "uniwm: virtual desktop %d no longer exists", (int)index);
    }

    WM_Error e = wm_vdesktop_switch(wm, span.desktops[index - 1]);
    if (e != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: switch failed (%d)", (int)e);
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
    lua_Integer index = lua_tointeger(L, lua_upvalueindex(1));
    WM_VDesktopSpan span = wm_vdesktops(wm);

    if (index < 1 || (size_t)index > span.count) {
        return luaL_error(L, "uniwm: virtual desktop %d no longer exists", (int)index);
    }

    lua_newtable(L);
    CollectVDeskWindows c = {.L = L, .table_index = lua_gettop(L), .count = 0};
    if (wm_vdesktop_windows(wm, span.desktops[index - 1], collect_vdesk_window, &c) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: failed to list windows on virtual desktop %d", (int)index);
    }

    return 1;
}

static int vdesk_size(lua_State *L) {
    WM *wm = current_wm(L);
    lua_Integer index = lua_tointeger(L, lua_upvalueindex(1));
    WM_VDesktopSpan span = wm_vdesktops(wm);

    if (index < 1 || (size_t)index > span.count) {
        return luaL_error(L, "uniwm: virtual desktop %d no longer exists", (int)index);
    }

    MC_Size2U size;
    if (wm_vdesktop_size(wm, span.desktops[index - 1], &size) != WM_ERROR_OK) {
        return luaL_error(L, "uniwm: failed to get virtual desktop size");
    }

    lua_createtable(L, 0, 2);
    lua_pushinteger(L, size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, size.height);
    lua_setfield(L, -2, "height");
    return 1;
}

static void push_vdesk(lua_State *L, WM *wm, WM_VDesktopSpan span, size_t i) {
    lua_createtable(L, 0, 4);

    MC_Str name = wm_vdesktop_name(wm, span.desktops[i]);
    lua_pushlstring(L, name.beg ? name.beg : "", MC_STR_LEN(name));
    lua_setfield(L, -2, "name");

    lua_pushinteger(L, (lua_Integer)(i + 1));
    lua_pushcclosure(L, vdesk_switch, 1);
    lua_setfield(L, -2, "switch");

    lua_pushinteger(L, (lua_Integer)(i + 1));
    lua_pushcclosure(L, vdesk_windows, 1);
    lua_setfield(L, -2, "windows");

    lua_pushinteger(L, (lua_Integer)(i + 1));
    lua_pushcclosure(L, vdesk_size, 1);
    lua_setfield(L, -2, "size");
}

static void push_vdesk_ptr(lua_State *L, WM *wm, WM_VDesktop *vdesk) {
    WM_VDesktopSpan span = wm_vdesktops(wm);
    for (size_t i = 0; i < span.count; i++) {
        if (span.desktops[i] == vdesk) {
            push_vdesk(L, wm, span, i);
            return;
        }
    }

    lua_pushnil(L);
}

static void vdesk_changed_cb(WM_VDesktop *current, WM_VDesktopChangeSource source, void *ctx) {
    LuaCallback *k = ctx;
    lua_State *L = k->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, k->ref);
    push_vdesk_ptr(L, wm_process(), current);
    lua_pushstring(L, source == WM_VDESKTOP_CHANGE_MANAGED ? "managed" : "external");

    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        fprintf(stderr, "uniwm: vdesktop:on_changed callback error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static int l_vdesk_on_changed(lua_State *L) {
    luaL_checktype(L, 2, LUA_TFUNCTION);
    WM *wm = current_wm(L);

    LuaCallback *k = NULL;
    if (mc_alloc(wm_allocator, sizeof(*k), (void **)&k) != MCE_OK) {
        return luaL_error(L, "uniwm.vdesktop:on_changed: out of memory");
    }
    k->L = L;
    lua_pushvalue(L, 2);
    k->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if (wm_vdesktop_on_changed(wm, vdesk_changed_cb, k) != WM_ERROR_OK) {
        lua_callback_free(k);
        return luaL_error(L, "uniwm.vdesktop:on_changed: failed");
    }

    return 0;
}

static int l_vdesk_list(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktopSpan span = wm_vdesktops(wm);

    lua_createtable(L, (int)span.count, 0);
    for (size_t i = 0; i < span.count; i++) {
        push_vdesk(L, wm, span, i);
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

    WM_VDesktopSpan span = wm_vdesktops(wm);
    for (size_t i = 0; i < span.count; i++) {
        if (span.desktops[i] == d) {
            push_vdesk(L, wm, span, i);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int l_vdesk_current(lua_State *L) {
    WM *wm = current_wm(L);
    WM_VDesktopSpan span = wm_vdesktops(wm);
    WM_VDesktop *cur = wm_vdesktop_current(wm);

    for (size_t i = 0; i < span.count; i++) {
        if (span.desktops[i] == cur) {
            push_vdesk(L, wm, span, i);
            return 1;
        }
    }

    lua_pushnil(L);
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

static int luaopen_libuniwm(lua_State *L) {
    static const luaL_Reg vdesk_fns[] = {
        { "list", l_vdesk_list },
        { "current", l_vdesk_current },
        { "create", l_vdesk_create },
        { "on_changed", l_vdesk_on_changed },
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

    return 1;
}

WM_Error uniwm_lua_open(lua_State *L) {
    if (L == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    luaL_requiref(L, "libuniwm", luaopen_libuniwm, 0);
    lua_pop(L, 1);

    mc_wm_lua_open(L, NULL);

    return WM_ERROR_OK;
}
