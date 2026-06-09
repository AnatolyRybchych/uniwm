local uniwm = require("libuniwm")

for i = #uniwm.virtual_desktop.list() + 1, 10 do
    uniwm.virtual_desktop.create("Desktop " .. i)
end

uniwm.supress_key("SUPER_L")

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
