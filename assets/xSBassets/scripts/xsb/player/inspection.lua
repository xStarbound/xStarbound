local module = {}
modules.inspection = module

-- FezzedOne: Display inspection messages in chat.
function module:init()
    message.setHandler("inspectionMessage", function(_, _, entityId, message)
        local entityName = "^red;<@" .. tostring(entityId) .. ">^reset;"
        if entityId == 0 then
            entityName = "^#888888;::^#aaa,set;"
        elseif world.entityExists(entityId) then
            local entType = world.entityType(entityId)
            if entType == "player" or entType == "npc" then
                entityName = world.entityName(entityId) or entityName
                entityName = "^#888,set;[" .. entityName .. "^reset,#888,set;]^#aaa,set;"
            else
                entityName = "^#888;::^reset,#aaa,set;"
            end
        else
            entityName = "^#888;::^reset,#aaa,set;"
        end
        chat.addMessage(nil, {
            context = { mode = "CommandResult" },
            nick = "",
            message = entityName .. " " .. message,
            showPane = false,
        })
        world.sendEntityMessage(player.id(), "newChatMessage", {
            mode = "Whisper",
            nickname = player.name(),
            connection = math.max(math.floor(((player.id() or 0) - 65535) / -65536), 0),
            text = entityName .. " " .. message,
        })
    end)
end
