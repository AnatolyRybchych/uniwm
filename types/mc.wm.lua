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

--- Events passed to `WM:on_event` callbacks are a tagged union discriminated by
--- `type`: narrow on `event.type` and each variant exposes exactly the fields it
--- carries. `window` is resolved lazily on first access (see `WM:on_event`); it is
--- absent for global events (`GLOBAL_*`).

--- `WINDOW_READY` / `WINDOW_REDRAW_REQUESTED` / `WINDOW_CLOSE_REQUESTED` /
--- `FOCUS_GAINED` / `FOCUS_LOST` — only the window.
---@class mcwm.Event.Window
---@field type "WINDOW_READY"|"WINDOW_REDRAW_REQUESTED"|"WINDOW_CLOSE_REQUESTED"|"FOCUS_GAINED"|"FOCUS_LOST"
---@field window mcwm.Window

---@class mcwm.Event.WindowResized
---@field type "WINDOW_RESIZED"
---@field window mcwm.Window
---@field new_size mcwm.Size

---@class mcwm.Event.WindowMoved
---@field type "WINDOW_MOVED"
---@field window mcwm.Window
---@field new_position mcwm.Position

---@class mcwm.Event.WindowStateChanged
---@field type "WINDOW_STATE_CHANGED"
---@field window mcwm.Window
---@field state mcwm.WindowState

--- `MOUSE_MOVED` / `MOUSE_ENTER` / `MOUSE_LEAVE`.
---@class mcwm.Event.MouseMove
---@field type "MOUSE_MOVED"|"MOUSE_ENTER"|"MOUSE_LEAVE"
---@field window mcwm.Window
---@field position mcwm.Position

--- `MOUSE_DOWN` / `MOUSE_UP`.
---@class mcwm.Event.MouseButton
---@field type "MOUSE_DOWN"|"MOUSE_UP"
---@field window mcwm.Window
---@field position mcwm.Position
---@field button string

---@class mcwm.Event.MouseWheel
---@field type "MOUSE_WHEEL"
---@field window mcwm.Window
---@field position mcwm.Position
---@field up integer
---@field right integer

--- `KEY_DOWN` / `KEY_UP`.
---@class mcwm.Event.Key
---@field type "KEY_DOWN"|"KEY_UP"
---@field window mcwm.Window
---@field key string

--- `TEXT_INPUT` / `PASTE_TEXT`.
---@class mcwm.Event.Text
---@field type "TEXT_INPUT"|"PASTE_TEXT"
---@field window mcwm.Window
---@field text string

--- `GLOBAL_KEY_DOWN` / `GLOBAL_KEY_UP`.
---@class mcwm.Event.GlobalKey
---@field type "GLOBAL_KEY_DOWN"|"GLOBAL_KEY_UP"
---@field key string

---@class mcwm.Event.GlobalMouseMove
---@field type "GLOBAL_MOUSE_MOVED"
---@field position mcwm.Position

--- `GLOBAL_MOUSE_DOWN` / `GLOBAL_MOUSE_UP`.
---@class mcwm.Event.GlobalMouseButton
---@field type "GLOBAL_MOUSE_DOWN"|"GLOBAL_MOUSE_UP"
---@field position mcwm.Position
---@field button string

---@class mcwm.Event.GlobalMouseWheel
---@field type "GLOBAL_MOUSE_WHEEL"
---@field position mcwm.Position
---@field up integer
---@field right integer

--- `NONE` / `RAW` / `MOUSE_CLICK` — no payload fields.
---@class mcwm.Event.Other
---@field type "NONE"|"RAW"|"MOUSE_CLICK"

---@alias mcwm.Event
---| mcwm.Event.Window
---| mcwm.Event.WindowResized
---| mcwm.Event.WindowMoved
---| mcwm.Event.WindowStateChanged
---| mcwm.Event.MouseMove
---| mcwm.Event.MouseButton
---| mcwm.Event.MouseWheel
---| mcwm.Event.Key
---| mcwm.Event.Text
---| mcwm.Event.GlobalKey
---| mcwm.Event.GlobalMouseMove
---| mcwm.Event.GlobalMouseButton
---| mcwm.Event.GlobalMouseWheel
---| mcwm.Event.Other

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

--- A reference to the shared window manager. The host (e.g. uniwm) owns the WM and
--- runs the event loop; scripts only ever hold references — there is intentionally no
--- `poll_event` here. Operations raise once the owner has destroyed the WM.
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

---@class mcwm.Subscription
local Subscription = {}

--- Stop receiving events for this subscription.
function Subscription:unsubscribe() end

--- Register a callback for events. `event_type` is an event type name (e.g.
--- `"GLOBAL_KEY_DOWN"`) or `nil` to match every event. The callback receives the
--- event as a table (see `mcwm.Event`); its `window` field is resolved lazily on
--- first access. Hold the returned subscription to keep it active and to be able to
--- `:unsubscribe()`. Callbacks fire from the host's event loop, which decides which
--- events to dispatch.
---@param event_type? mcwm.EventType
---@param callback fun(event: mcwm.Event)
---@return mcwm.Subscription
function WM:on_event(event_type, callback) end

--- Release this reference to the WM. Does not tear down the WM itself — the owner
--- keeps it alive until the last reference is dropped.
function WM:destroy() end

---@class mcwm
local mcwm = {}

--- Get a reference to the shared window manager (lazily created on first use if no
--- host has created one).
---@param impl? string # require a specific backend, e.g. "WIN32"
---@return mcwm.WM
function mcwm.resolve(impl) end

return mcwm
