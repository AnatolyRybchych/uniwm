#include <uniwm-lua/uniwm-lua.h>

#include <lua.h>
#include <lauxlib.h>

#include <mc/data/str.h>

#include <uniwm/wm.h>
#include <uniwm/target.h>

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

static void push_vdesk(lua_State *L, WM *wm, WM_VDesktopSpan span, size_t i) {
    lua_createtable(L, 0, 2);

    MC_Str name = wm_vdesktop_name(wm, span.desktops[i]);
    lua_pushlstring(L, name.beg ? name.beg : "", MC_STR_LEN(name));
    lua_setfield(L, -2, "name");

    lua_pushinteger(L, (lua_Integer)(i + 1));
    lua_pushcclosure(L, vdesk_switch, 1);
    lua_setfield(L, -2, "switch");
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

static int luaopen_libuniwm(lua_State *L) {
    static const luaL_Reg vdesk_fns[] = {
        { "list", l_vdesk_list },
        { "current", l_vdesk_current },
        { NULL, NULL },
    };

    lua_createtable(L, 0, 3);

    luaL_newlib(L, vdesk_fns);
    lua_setfield(L, -2, "virtual_desktop");

    const WM_TargetInterface *ti = wm_default_target();
    if (ti && ti->suppress_key) {
        lua_pushcfunction(L, l_supress_key);
        lua_setfield(L, -2, "supress_key");
    }
    if (ti && ti->unsuppress_key) {
        lua_pushcfunction(L, l_unsupress_key);
        lua_setfield(L, -2, "unsupress_key");
    }

    return 1;
}

WM_Error uniwm_lua_open(lua_State *L) {
    if (L == NULL) {
        return WM_ERROR_INVALID_ARGUMENT;
    }

    luaL_requiref(L, "libuniwm", luaopen_libuniwm, 0);
    lua_pop(L, 1);

    return WM_ERROR_OK;
}
