-- Preprocessing script that adds "xClient v[version]" to the window title.

local oldWindowTitle = assets.json("/client.config:windowTitle")
local postProcPatch = {
    windowTitle = oldWindowTitle .. " :: xClient v" .. xsb.version() .. ""
}
assets.add("/client.config.postproc", postProcPatch)
assets.patch("/client.config", "/client.config.postproc")