local uniwm = require("libuniwm")
local mcwm = require("mc.wm")

local wm = mcwm.resolve()

local function layout_vdesk (desktop, managed)
    print(string.format('DESKTOP CHANGED to %s [%s]', desktop.name , managed))
    local windows = desktop:windows()
    local count = #windows
    if count == 0 then
        return
    end

    local size = desktop:size()
    local width = math.floor(size.width / count)
    for i, window in ipairs(windows) do
        pcall(function()
            window:set_state("normal")
            window:set_rect({ x = (i - 1) * width, y = 0, width = width, height = size.height }, "decorated")
        end)
    end
end

uniwm.vdesktop:on_changed(layout_vdesk)
layout_vdesk(uniwm.vdesktop:current(), true)
for i = #uniwm.vdesktop:list() + 1, 10 do
    local desk_name = "Desktop " .. i
    uniwm.vdesktop:create(desk_name)
end

uniwm.supress_key("SUPER_L")

uniwm.supress_key("SUPER_L + SHIFT_L + C")
uniwm.register_keybind("SUPER_L + SHIFT_L + C", function()
    local hovered = wm:get_hovered_window()
    if hovered then
        print(string.format("Close window '%s' [%s]", hovered:get_title(), hovered:get_state()))
        hovered:close()
    end
end)

local list = uniwm.vdesktop:list()

for i = 1, 10 do
    local switch_vdesk_hotkey = "SUPER_L + " .. (i % 10)
    local move_window_hotkey = "SUPER_L + SHIFT_L + " .. (i % 10)

    uniwm.supress_key(switch_vdesk_hotkey)
    uniwm.register_keybind(switch_vdesk_hotkey, function()
        if list[i] then
            list[i]:switch()
        end
    end)

    uniwm.supress_key(move_window_hotkey)
    uniwm.register_keybind(move_window_hotkey, function()
        local window = wm:get_focused_window()
        if list[i] and window then
            list[i]:move_window(window)
        end
    end)
end
