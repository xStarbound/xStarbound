--- FezzedOne's StarCustomChat Compatibility Patch for xStarbound ---

local patchHeader = assets.bytes("/starcustomchat-xsb-compat.header.lua")
local patchFooter = assets.bytes("/starcustomchat-xsb-compat.footer.lua")
local soundStatusPatchFooter = assets.bytes("/starcustomchat-xsb-compat.voice-status.lua")
local chatOpenerFooter = assets.bytes("/starcustomchat-xsb-compat.chatopener.lua")
local chatHandlerFooter = assets.bytes("/starcustomchat-xsb-compat.chathandler.lua")
local biggerChatPatch = assets.bytes("/starcustomchat-biggerchat-fix.lua")

local sccChatOpenerScript = "/scripts/starcustomchat/chatopener.lua"
local sccGuiScript = "/interface/scripted/starcustomchat/base/starcustomchatgui.lua"
local sccTalkingEffectScript =
    "/interface/scripted/starcustomchat/plugins/sounds/stats/effects/scctalking/scctalking.lua"
local sccChatRevealScript = "/interface/scripted/starcustomchatreveal/chatreveal.lua"
local sccBiggerChatScript = "/interface/BiggerChat/biggerchatv2.lua"
local otherSccScriptsToPatch = {
    "/interface/scripted/starcustomchat/plugins/sounds/sounds.lua",
    "/interface/scripted/starcustomchat/plugins/sounds/soundssettings.lua",
}

if xsb then
    if assets.exists(sccChatOpenerScript) then
        local baseScript = assets.bytes(sccChatOpenerScript)
        local patchedScript = patchHeader .. baseScript .. patchFooter .. chatOpenerFooter
        assets.erase(sccChatOpenerScript)
        assets.add(sccChatOpenerScript, patchedScript)
    end

    if assets.exists(sccGuiScript) then
        local baseScript = assets.bytes(sccGuiScript)
        local patchedScript = patchHeader .. baseScript .. patchFooter .. chatHandlerFooter
        assets.erase(sccGuiScript)
        assets.add(sccGuiScript, patchedScript)
    end

    if assets.exists(sccTalkingEffectScript) then
        local baseScript = assets.bytes(sccTalkingEffectScript)
        local patchedScript = patchHeader .. baseScript .. soundStatusPatchFooter
        assets.erase(sccTalkingEffectScript)
        assets.add(sccTalkingEffectScript, patchedScript)
    end

    if assets.exists(sccChatRevealScript) then
        local baseScript = assets.bytes(sccChatRevealScript)
        local patchedScript = patchHeader .. baseScript .. patchFooter
        assets.erase(sccChatRevealScript)
        assets.add(sccChatRevealScript, patchedScript)
    end

    if assets.exists(sccBiggerChatScript) then
        local baseScript = assets.bytes(sccBiggerChatScript)
        local patchedScript = baseScript .. biggerChatPatch
        assets.erase(sccBiggerChatScript)
        assets.add(sccBiggerChatScript, patchedScript)
    end

    for _, path in ipairs(otherSccScriptsToPatch) do
        if assets.exists(path) then
            local baseScript = assets.bytes(path)
            local patchedScript = patchHeader .. baseScript
            assets.erase(path)
            assets.add(path, patchedScript)
        end
    end
end
