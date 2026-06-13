---@meta libuniwm

--- A virtual desktop. Obtained from `vdesktop:list()`/`:current()`/`:create()`
--- or the `on_changed` callback. The same desktop always yields the same table.
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

--- How a desktop change was triggered: "managed" (an explicit uniwm switch) or
--- "external" (another API, e.g. WIN+TAB — not yet detected).
---@alias libuniwm.ChangeSource "managed"|"external"

---@class libuniwm.VirtualDesktop
local VirtualDesktop = {}

---@return libuniwm.Desktop[]
function VirtualDesktop:list() end

---@return libuniwm.Desktop? # nil if it cannot be determined
function VirtualDesktop:current() end

---@param name string
---@return libuniwm.Desktop?
function VirtualDesktop:create(name) end

--- Subscribe to desktop-change notifications. Multiple subscribers allowed.
---@param fn fun(desktop: libuniwm.Desktop, source: libuniwm.ChangeSource)
function VirtualDesktop:on_changed(fn) end

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
--- specific matching combo fires.
---@param spec string
---@param fn fun()
function libuniwm.register_keybind(spec, fn) end

---@param spec string
function libuniwm.unregister_keybind(spec) end

--- Subscribe to window creation. `fn` is called with each new **root application
--- window** (an `mc.wm` window) when it is first shown — child/control windows and
--- tool/infra windows (tooltips, IME, OLE helpers) are excluded. Registering this
--- turns the process into a daemon (the event loop runs even without keybinds).
---@param fn fun(window: mcwm.Window)
function libuniwm.on_window_created(fn) end

--- Subscribe to window destruction. `fn` is called with each window as it is
--- destroyed (an `mc.wm` window). The native handle is already gone, so query its
--- identity (e.g. to match a previously tracked window) rather than its live state.
---@param fn fun(window: mcwm.Window)
function libuniwm.on_window_destroyed(fn) end

return libuniwm
