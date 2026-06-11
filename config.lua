local uniwm = require("libuniwm")

for i = #uniwm.virtual_desktop.list() + 1, 10 do
    uniwm.virtual_desktop.create("Desktop " .. i)
end

uniwm.supress_key("SUPER_L")

local mcwm = require("mc.wm")
local wm = mcwm.resolve()
local window = wm:create_window({ title = "Test", size = { width = 640, height = 480 } })

uniwm.supress_key("SUPER_L + SHIFT_L + C")
uniwm.register_keybind("SUPER_L + SHIFT_L + C", function()
    local focused = wm:get_focused_window()
    if focused then
        print(string.format("Close window '%s' [%s]", focused:get_title(), focused:get_state()))
        focused:close()
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
