-- Post-processing script that overrides any window title changes made by mods.

local postProcPatch = '{"windowTitle": "xClient v' .. xsb.version() .. '"}'

assets.add("/client.config.postproc", postProcPatch)
assets.patch("/client.config", "/client.config.postproc")
