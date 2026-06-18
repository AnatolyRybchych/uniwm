---@meta libuniwm

--- Window-manager events (desktop changed, window created/destroyed/focused) are NOT
--- delivered through this module — subscribe via `mc.wm`:
---   `require("mc.wm").resolve():on_event("UNIWM.VDESKTOP_CHANGED", fn)`
---   (also `"UNIWM.WINDOW_CREATED"` / `"UNIWM.WINDOW_DESTROYED"` / `"UNIWM.WINDOW_FOCUSED"`).
--- Their payloads carry `window_id` (+ a lazily-resolved `window`) or `desktop`/`source`;
--- see `mcwm.EventType` / `mcwm.Event`.

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
local libuniwm = {}

--- Swallow a key combo so it never reaches applications. `spec` is up to 4
--- `mc_key` names joined by "+", e.g. "SUPER_L + SHIFT_L + R".
---@param spec string
function libuniwm.supress_key(spec) end

---@param spec string
function libuniwm.unsupress_key(spec) end

--- Run `fn` on a fresh press of `spec` (does not swallow). Only the most
--- specific matching combo fires. Re-registering a combo replaces its callback.
---@param spec string
---@param fn fun()
function libuniwm.register_keybind(spec, fn) end

---@param spec string
function libuniwm.unregister_keybind(spec) end

--- Resolve a native window identity (the win32 HWND value, as carried by the
--- `UNIWM.WINDOW_*` events' `window_id`) into an `mc.wm` window, or nil if it
--- cannot be resolved (e.g. the window is gone).
---@param id integer
---@return mcwm.Window?
function libuniwm.window(id) end

return libuniwm
