local uniwm = require("libuniwm")

local desktops = uniwm.virtual_desktop.list()

for i = 1, 10 do
    local combo = "SUPER_L + " .. (i % 10)
    uniwm.supress_key(combo)
    uniwm.register_keybind(combo, function()
        local list = uniwm.virtual_desktop.list()
        if list[i] then
            list[i]:switch()
        end
    end)
end
