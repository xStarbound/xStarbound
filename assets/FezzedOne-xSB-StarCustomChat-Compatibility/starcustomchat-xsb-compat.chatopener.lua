
-- < END OF FOOTER > --

local base_init = init

require("/scripts/vec2.lua")
require("/interface/scripted/starcustomchat/base/starcustomchatutils.lua")

function init(...) -- Chat portraits require special handling because they return a value.
    message.setHandler(
        "icc_request_player_portrait",
        simpleHandler(function()
            if player.id() and world.entityExists(player.id()) then
                local sccConfig = root.assetJson("/interface/scripted/starcustomchat/base/chat.config")
                return {
                    portrait = player.getProperty("icc_custom_portrait")
                        or starcustomchat.utils.clearPortraitFromInvisibleLayers(
                            world.entityPortrait(player.id(), "full")
                        ),
                    type = "UPDATE_PORTRAIT",
                    entityId = player.id(),
                    connection = player.id() // -65536,
                    settings = player.getProperty("icc_portrait_settings") or {
                        offset = sccConfig.defaultPortraitOffset,
                        scale = sccConfig.defaultPortraitScale,
                    },
                    uuid = player.uniqueId(),
                }
            end
        end)
    )
    message.setHandler("icc_is_chat_open", simpleHandler(function() -- Also needs special handling because it returns a value.
      return shared.chatIsOpen
    end))
    require "/scripts/starextensions/lib/chat_callback.lua"
    setChatMessageHandler(receiveMessage) -- Set up this handler early so that login messages aren't lost.
    return base_init(...)
end

local base_receiveMessage = receiveMessage

function receiveMessage(message)
    if not self.chatHidden then
        world.sendEntityMessage(player.id(), "icc_xSB_addChat", message or jobject())
    end
    base_receiveMessage(message or jobject())
end
