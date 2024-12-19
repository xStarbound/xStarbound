-- Needed for StarExtensions library emulation.

function setChatMessageHandler(handlerFunc)
    if xsb and message and message.setHandler then
        local wrapperFunc = function(...)
            local handler = handlerFunc
            return handler(...)
        end
        message.setHandler({
            localOnly = true,
            passName = false,
            name = "newChatMessage",
        }, wrapperFunc)
    end
    return function()
        -- message.setHandler({
        --     localOnly = true,
        --     passName = false,
        --     name = "newChatMessage",
        -- }, function() return nil end)
    end
end
