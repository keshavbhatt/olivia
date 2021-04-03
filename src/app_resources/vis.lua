-- audio visualization

local opts = {
    mode = "force",
    -- off              disable visualization
    -- noalbumart       enable visualization when no albumart and no video
    -- novideo          enable visualization when no video
    -- force            always enable visualization

    name = "showcqtbar",
    -- off
    -- showcqtbar

    quality = "medium",
    -- verylow
    -- low
    -- medium
    -- high
    -- veryhigh

    height = 5,
    -- [4 .. 12]
}

-- key bindings
-- cycle visualizer
local cycle_key = "c"

if not (mp.get_property("options/lavfi-complex", "") == "") then
    return
end

local visualizer_name_list = {
    "off",
    "showcqtbar",
}

local options = require 'mp.options'
local msg     = require 'mp.msg'

options.read_options(opts)
opts.height = math.min(12, math.max(4, opts.height))
opts.height = math.floor(opts.height)

local function get_visualizer(name, quality)
    local w, h, fps

    if quality == "verylow" then
        w = 640
        fps = 25
    elseif quality == "low" then
        w = 960
        fps = 25
    elseif quality == "medium" then
        w = 1280
        fps = 60
    elseif quality == "high" then
        w = 1920
        fps = 60
    elseif quality == "veryhigh" then
        w = 2560
        fps = 60
    else
        msg.log("error", "invalid quality")
        return ""
    end

    h = w * opts.height / 16

     

  if name == "showcqtbar" then
        local axis_h = math.ceil(w * 12 / 1920) * 4

        return "[aid1] asplit [ao]," ..
            "afifo, aformat     = channel_layouts = stereo," ..
            "firequalizer       =" ..
                "gain           = '1.4884e8 * f*f*f / (f*f + 424.36) / (f*f + 1.4884e8) / sqrt(f*f + 25122.25)':" ..
                "scale          = linlin:" ..
                "wfunc          = tukey:" ..
                "zero_phase     = on:" ..
                "fft2           = on," ..
            "showcqt            =" ..
                "fps            =" .. fps .. ":" ..
                "size           =" .. w .. "x" .. (h + axis_h)/2 .. ":" ..
                "count          = 1:" ..
                "csp            = smpte240m:" ..
                "bar_g          = 2:" ..
                "sono_g         = 4:" ..
                "bar_v          = 9:" ..
                "sono_v         = 17:" ..
                "sono_h         = 0:" ..
		"axis           = 0:" ..
                "tc             = 0.33:" ..
                "attack         = 0.033:" ..
                "tlength        = 'st(0,0.17); 384*tc / (384 / ld(0) + tc*f /(1-ld(0))) + 384*tc / (tc*f / ld(0) + 384 /(1-ld(0)))'," ..
            "format             = yuv420p," ..
            "split [v0]," ..
            "crop               =" ..
                "h              =" .. (h - axis_h)/2 .. ":" ..
                "y              = 0," ..
            "vflip [v1];" ..
            "[v0][v1] vstack [vo]"

    elseif name == "off" then
        return "[aid1] afifo [ao]"
    end

    msg.log("error", "invalid visualizer name")
    return ""
end

local function select_visualizer(atrack, vtrack, albumart)
    if opts.mode == "off" then
        return ""
    elseif opts.mode == "force" then
        return get_visualizer(opts.name, opts.quality)
    elseif opts.mode == "noalbumart" then
        if albumart == 0 and vtrack == 0 then
            return get_visualizer(opts.name, opts.quality)
        end
        return ""
    elseif opts.mode == "novideo" then
        if vtrack == 0 then
            return get_visualizer(opts.name, opts.quality)
        end
        return ""
    end

    msg.log("error", "invalid mode")
    return ""
end

local function visualizer_hook()
    local count = mp.get_property_number("track-list/count", -1)
    local atrack = 0
    local vtrack = 0
    local albumart = 0
    if count <= 0 then
        return
    end
    for tr = 0,count-1 do
        if mp.get_property("track-list/" .. tr .. "/type") == "audio" then
            atrack = atrack + 1
        else
            if mp.get_property("track-list/" .. tr .. "/type") == "video" then
                if mp.get_property("track-list/" .. tr .. "/albumart") == "yes" then
                    albumart = albumart + 1
                else
                    vtrack = vtrack + 1
                end
            end
        end
    end

    mp.set_property("options/lavfi-complex", select_visualizer(atrack, vtrack, albumart))
end

mp.add_hook("on_preloaded", 50, visualizer_hook)

local function cycle_visualizer()
    local i, index = 1
    for i = 1, #visualizer_name_list do
        if (visualizer_name_list[i] == opts.name) then
            index = i + 1
            if index > #visualizer_name_list then
                index = 1
            end
            break
        end
    end
    opts.name = visualizer_name_list[index]
    visualizer_hook()
end

mp.add_key_binding(cycle_key, "cycle-visualizer", cycle_visualizer)
