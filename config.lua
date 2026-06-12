local uniwm = require("libuniwm")

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
        window:set_state("normal")
        window:set_rect({ x = (i - 1) * width, y = 0, width = width, height = size.height }, "decorated")
    end
end

uniwm.virtual_desktop.on_changed(layout_vdesk)
layout_vdesk(uniwm.virtual_desktop.current(), true)

for i = #uniwm.virtual_desktop.list() + 1, 10 do
    uniwm.virtual_desktop.create("Desktop " .. i)
end

uniwm.supress_key("SUPER_L")

local mcwm = require("mc.wm")
local wm = mcwm.resolve()

uniwm.supress_key("SUPER_L + SHIFT_L + C")
uniwm.register_keybind("SUPER_L + SHIFT_L + C", function()
    local hovered = wm:get_hovered_window()
    if hovered then
        print(string.format("Close window '%s' [%s]", hovered:get_title(), hovered:get_state()))
        hovered:close()
    end
end)

local list = uniwm.virtual_desktop.list()

for i = 1, 10 do
    local combo = "SUPER_L + " .. (i % 10)
    uniwm.supress_key(combo)
    uniwm.register_keybind(combo, function()
        if list[i] then
            list[i]:switch()
        end
    end)
end
