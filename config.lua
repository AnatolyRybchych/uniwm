local uniwm = require("libuniwm")

for i = #uniwm.virtual_desktop.list() + 1, 10 do
    uniwm.virtual_desktop.create("Desktop " .. i)
end

uniwm.supress_key("SUPER_L")

local mcwm = require("mc.wm")
local wm = mcwm.resolve()

for _, window in ipairs(wm:get_all_windows()) do
    print(string.format('[%s]\t%s', window:get_state(), window:get_title()))
end

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
