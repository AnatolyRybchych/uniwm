local uniwm = require("libuniwm")

local desktops = uniwm.virtual_desktop.list()
local current = uniwm.virtual_desktop.current()

print(string.format("uniwm: %d desktop(s)", #desktops))

for i, vdesk in ipairs(desktops) do
    local marker = (current and vdesk.name == current.name) and " (current)" or ""
    print(string.format("  %d. %s%s", i, vdesk.name, marker))
end

desktops[1]:switch()
