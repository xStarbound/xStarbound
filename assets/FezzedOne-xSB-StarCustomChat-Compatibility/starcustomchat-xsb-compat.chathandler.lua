
-- < END OF FOOTER > --

local base_init = init

function init()
    local base_chat_command = chat.command
    chat.command = function(command)
        local result = base_chat_command(command)
        if result and self.customChat then
            for _, line in ipairs(result) do -- Huge kludge here just to get client-side command results to show up in the chat box.
                self.customChat:addMessage({
                    text = line,
                    connection = 0,
                    mode = "CommandResult",
                })
            end
        end
        return result
    end

    return base_init()
end

local base_registerCallbacks = registerCallbacks

function registerCallbacks(...)
    shared.setMessageHandler("icc_xSB_addChat", localHandler(function(message) self.customChat:addMessage(message) end))

    return base_registerCallbacks(...)
end
