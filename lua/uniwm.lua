--- uniwm — typed proxy around the native uniwm Lua bindings.
--- Loads the native module (`libuniwm-lua.dll`/`.so`), re-exports it with LuaLS type
--- annotations, and returns the module. Install to `<luadir>/uniwm.lua` so
--- `require("uniwm")` resolves here and then loads `libuniwm-lua` underneath (whose
--- `luaopen_libuniwm` registers the windows target and the `mc.wm` module).
---
--- (When embedded by uniwm.exe, `uniwm` is registered in C and preloaded into
--- `package.loaded` before any script runs, so this file is not loaded there; it serves
--- the standalone interpreter case. NOTE: standalone, keybinds and events do NOT fire —
--- those need uniwm.exe's poll/dispatch loop; one-shot desktop/window queries do work.)
---
--- Window-manager events (desktop changed, window created/destroyed/focused) are NOT
--- delivered through this module — subscribe via `mc.wm`:
---   `require("mc.wm").resolve():on_event("UNIWM.VDESKTOP_CHANGED", fn)`
---   (also `"UNIWM.WINDOW_CREATED"` / `"UNIWM.WINDOW_DESTROYED"` / `"UNIWM.WINDOW_FOCUSED"`).
--- Their payloads carry `window_id` (+ a lazily-resolved `window`) or `desktop`/`source`;
--- see `mcwm.EventType` / `mcwm.Event`.

local native = require("libuniwm-lua")

--- A virtual desktop. Obtained from `vdesktop:list()`/`:current()`/`:create()`.
--- The same desktop always yields the same table.
---@class libuniwm.Desktop
---@field name string
local Desktop = {}

--- Switch to this desktop.
function Desktop:switch() end

--- Application windows on this desktop (system/ghost windows excluded on the
--- current desktop). Each is an `mc.wm` window.
---@return mcwm.Window[]
function Desktop:windows() end

--- Work area (tileable region, excludes the taskbar) of the desktop.
---@return mcwm.Size
function Desktop:size() end

--- Move the given window onto this desktop.
---@param window mcwm.Window
function Desktop:move_window(window) end

--- The process-wide `vdesktop` singleton. Call its methods with `:`.
---@class libuniwm.VirtualDesktop
local VirtualDesktop = {}

--- All virtual desktops, in order. Use `#list` for the count.
---@return libuniwm.Desktop[]
function VirtualDesktop:list() end

--- The current desktop, or nil if it cannot be determined.
---@return libuniwm.Desktop? # nil if it cannot be determined
function VirtualDesktop:current() end

--- Create a new virtual desktop named `name` and return it.
---@param name string
---@return libuniwm.Desktop?
function VirtualDesktop:create(name) end

---@class libuniwm
---@field vdesktop libuniwm.VirtualDesktop
local M = {}

--- The `vdesktop` singleton, re-exported from the native module.
M.vdesktop = native.vdesktop

--- Swallow a key combo so it never reaches applications. `spec` is up to 4
--- `mc_key` names joined by "+", e.g. "SUPER_L + SHIFT_L + R".
---@param spec string
function M.supress_key(spec)
    native.supress_key(spec)
end

---@param spec string
function M.unsupress_key(spec)
    native.unsupress_key(spec)
end

--- Run `fn` on a fresh press of `spec` (does not swallow). Only the most
--- specific matching combo fires. Re-registering a combo replaces its callback.
---@param spec string
---@param fn fun()
function M.register_keybind(spec, fn)
    native.register_keybind(spec, fn)
end

---@param spec string
function M.unregister_keybind(spec)
    native.unregister_keybind(spec)
end

--- Resolve a native window identity (the win32 HWND value, as carried by the
--- `UNIWM.WINDOW_*` events' `window_id`) into an `mc.wm` window, or nil if it
--- cannot be resolved (e.g. the window is gone).
---@param id integer
---@return mcwm.Window?
function M.window(id)
    return native.window(id)
end

return M
