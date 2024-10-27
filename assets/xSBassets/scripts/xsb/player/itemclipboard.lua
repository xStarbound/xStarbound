local module = {}
modules.itemclipboard = module

-- FezzedOne: Allow items to be copied and pasted from the clipboard.

local function resolveItem(itemJson)
    local success, itemConfigOrError = pcall(root.itemConfig, itemJson)
    if success then
        return itemConfigOrError.parameters.shortdescription
            or itemConfigOrError.config.shortdescription
            or itemConfigOrError.config.itemName,
            itemJson
    else
        return itemConfigOrError, null
    end
end

function module:update(dt)
    if entity.id() == world.primaryPlayer() then
        if input.bindDown("xSB", "copyItem") then
            local itemToCopy = player.primaryHandItem() or player.altHandItem()
            if itemToCopy then
                clipboard.setText(sb.printJson(itemToCopy, 1))
                if clipboard.hasText() then
                    local itemName, validConfig = resolveItem(itemToCopy)
                    itemName = validConfig and itemName or "<invalid item>"
                    local queueMessage = "^white,set;Copied ^orange,set;" .. itemName .. "^reset,white,set; to clipboard."
                    interface.queueMessage(queueMessage)
                else
                    local queueMessage = "^red;Unable to copy item to clipboard."
                    interface.queueMessage(queueMessage)
                end
            else
                local queueMessage = "^red;No item copied."
                interface.queueMessage(queueMessage)
            end
        end

        if input.bindDown("xSB", "cutItem") then
            local itemToCopy = player.swapSlotItem()
            if itemToCopy then
                clipboard.setText(sb.printJson(itemToCopy, 1))
                local itemName = resolveItem(itemToCopy)
                if clipboard.hasText() then
                    player.setSwapSlotItem(null)
                    local queueMessage = "^white,set;Cut ^orange,set;" .. itemName .. "^reset,white,set; to clipboard."
                    interface.queueMessage(queueMessage)
                else
                    local queueMessage = "^red;Unable to cut item to clipboard."
                    interface.queueMessage(queueMessage)
                end
            else
                local queueMessage = "^red;No item cut."
                interface.queueMessage(queueMessage)
            end
        end

        if input.bindDown("xSB", "pasteItem") then
            if clipboard.hasText() then
                local rawItemText = clipboard.getText()
                local parseSuccess, itemJson = pcall(sb.parseJson, rawItemText)
                if parseSuccess then
                    if itemJson then
                        local itemNameOrError, checkedItemJson = resolveItem(itemJson)
                        if checkedItemJson then
                            local existingSwapItem = player.swapSlotItem()
                            if existingSwapItem then player.giveItem(existingSwapItem) end
                            local swapStatus, swapError = pcall(player.setSwapSlotItem, checkedItemJson)
                            if swapStatus then
                                local queueMessage = "^white,set;Spawned ^orange,set;"
                                    .. itemNameOrError
                                    .. "^reset,white,set; from clipboard."
                                interface.queueMessage(queueMessage)
                            else
                                sb.logInfo(
                                    "[xSB] Error while spawning item on clipboard: %s",
                                    sb.print(swapError)
                                )
                                local queueMessage = "^red;Error while spawning item; check log."
                                interface.queueMessage(queueMessage)
                            end
                        else
                            sb.logInfo(
                                "[xSB] Error while resolving item on clipboard: %s",
                                sb.print(itemNameOrError)
                            )
                            local queueMessage = "^red;Error while pasting item; check log."
                            interface.queueMessage(queueMessage)
                        end
                    else
                        local queueMessage = "^red;No valid JSON on clipboard."
                        interface.queueMessage(queueMessage)
                    end
                else
                    local queueMessage = "^red;No valid JSON on clipboard."
                    interface.queueMessage(queueMessage)
                end
            else
                local queueMessage = "^red;No valid JSON on clipboard."
                interface.queueMessage(queueMessage)
            end
        end
    end
end
