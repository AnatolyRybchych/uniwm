---@meta mc.wm

---@class mcwm.Size
---@field width integer
---@field height integer

---@class mcwm.Position
---@field x integer
---@field y integer

---@class mcwm.Rect
---@field x integer
---@field y integer
---@field width integer
---@field height integer

---@alias mcwm.Area "window"|"decorated"|"drawable"
---@alias mcwm.WindowState "normal"|"minimized"|"maximized"|"fullscreen"

--- Event type names (as `mc_wm_event_type_str` emits / `mc_wm_event_type_from_str`
--- accepts). `"NONE"` matches every event (but prefer `nil` in `WM:on_event`).
---@alias mcwm.EventType
---| "NONE"
---| "RAW"
---| "WINDOW_READY"
---| "WINDOW_RESIZED"
---| "WINDOW_MOVED"
---| "WINDOW_REDRAW_REQUESTED"
---| "WINDOW_CLOSE_REQUESTED"
---| "WINDOW_STATE_CHANGED"
---| "FOCUS_GAINED"
---| "FOCUS_LOST"
---| "MOUSE_MOVED"
---| "MOUSE_DOWN"
---| "MOUSE_UP"
---| "MOUSE_CLICK"
---| "MOUSE_ENTER"
---| "MOUSE_LEAVE"
---| "MOUSE_WHEEL"
---| "KEY_DOWN"
---| "KEY_UP"
---| "TEXT_INPUT"
---| "PASTE_TEXT"
---| "GLOBAL_KEY_DOWN"
---| "GLOBAL_KEY_UP"
---| "GLOBAL_MOUSE_MOVED"
---| "GLOBAL_MOUSE_DOWN"
---| "GLOBAL_MOUSE_UP"
---| "GLOBAL_MOUSE_WHEEL"

---@class mcwm.WindowOpts
---@field title? string
---@field size? mcwm.Size
---@field position? mcwm.Position
---@field state? mcwm.WindowState

---@class mcwm.Event
---@field type mcwm.EventType
---@field key? integer

---@class mcwm.Window
local Window = {}

---@param title string
---@return mcwm.Window self
function Window:set_title(title) end

---@param size mcwm.Size
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_size(size, area) end

---@param position mcwm.Position
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_position(position, area) end

---@param rect mcwm.Rect
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_rect(rect, area) end

---@param state mcwm.WindowState
---@return mcwm.Window self
function Window:set_state(state) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Size
function Window:get_size(area) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Position
function Window:get_position(area) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Rect
function Window:get_rect(area) end

---@return string
function Window:get_title() end

---@return mcwm.WindowState
function Window:get_state() end

---@return boolean
function Window:is_alive() end

--- Whether this is a shell/system window (taskbar, desktop, cloaked UWP
--- CoreWindow, tool window) rather than a user application window.
---@return boolean
function Window:is_system() end

--- Politely ask the window to close (foreign: WM_CLOSE; managed: destroy).
function Window:close() end

--- Give the window input focus / bring it to the foreground.
---@return mcwm.Window self
function Window:focus() end

--- Destroy a window this WM owns (created via `WM:create_window`).
function Window:destroy() end

---@class mcwm.WM
local WM = {}

---@param opts? mcwm.WindowOpts
---@return mcwm.Window
function WM:create_window(opts) end

---@return mcwm.Window? # nil if there is no focused window
function WM:get_focused_window() end

---@return mcwm.Window? # nil if there is no hovered window
function WM:get_hovered_window() end

---@return mcwm.Window[]
function WM:get_all_windows() end

---@return mcwm.Event? # nil if the queue is empty
function WM:poll_event() end

---@class mcwm.Subscription
local Subscription = {}

--- Stop receiving events for this subscription.
function Subscription:unsubscribe() end

--- Register a callback for events. `event_type` is an event type name (e.g.
--- `"GLOBAL_KEY_DOWN"`) or `nil` to match every event. The callback currently
--- receives no arguments. Hold the returned subscription to keep it active and to
--- be able to `:unsubscribe()`. Callbacks fire from the host's event loop, which
--- decides which events to dispatch.
---@param event_type? mcwm.EventType
---@param callback fun()
---@return mcwm.Subscription
function WM:on_event(event_type, callback) end

function WM:destroy() end

---@class mcwm
local mcwm = {}

--- Resolve the shared window manager (creates it on first use).
---@param impl? string # require a specific backend, e.g. "WIN32"
---@return mcwm.WM
function mcwm.resolve(impl) end

return mcwm
