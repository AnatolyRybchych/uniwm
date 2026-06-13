local uniwm = require("libuniwm")
local mcwm = require("mc.wm")

local wm = mcwm.resolve()

local function filter(table, predicate)
    local res = {}
    for k, v in pairs(table) do
        if predicate(v, k) then
            res[k] = v
        end
    end

    return res
end

local function map(table, transform)
    local res = {}
    for k, v in pairs(table) do
        res[k] = transform(v, k)
    end

    return res
end

local function list(tab)
    local res = {}
    for k, v in pairs(tab) do
        table.insert(res, v)
    end

    return res
end

local function dump(value, indent)
    indent = indent or ""
    if type(value) ~= "table" then
        print(indent .. tostring(value))
        return
    end

    for k, v in pairs(value) do
        if type(v) == "table" then
            print(indent .. tostring(k) .. ":")
            dump(v, indent .. "  ")
        else
            print(indent .. tostring(k) .. " = " .. tostring(v))
        end
    end
end

local function layout_vdesk (desktop, managed)    
    print('------------------------------')
    print(string.format('DESKTOP CHANGED to %s [%s]', desktop.name , managed))

    local windows = desktop:windows()
    windows = list(filter(windows, function (w) return not w:is_system() end))

    if #windows == 0 then
        return
    end

    for k, window in pairs(windows) do
        print('  ' .. window:get_title())
    end

    local size = desktop:size()
    local width = math.floor(size.width / #windows)
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
            local window = wm:get_hovered_window()
            if window then
                window:focus()
            end
        end
    end)

    uniwm.supress_key(move_window_hotkey)
    uniwm.register_keybind(move_window_hotkey, function()
        local window = wm:get_hovered_window()
        if list[i] and window then
            list[i]:move_window(window)
        end
    end)
end

uniwm.on_window_created(function (window)
    print(window:get_title())
end)

uniwm.on_window_destroyed(function (window)
    print(window:get_title())
end)
