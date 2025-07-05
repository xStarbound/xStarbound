local loadedModPaths = assets.loadedSources()
local loadedMods = {}
for loadedModPaths as loadedMod do
    table.insert(loadedMods, assets.sourceMetadata(loadedMod).name ?? loadedMod)
end

local logInfo = |modName| -> sb.logInfo($"[xSB] Detected and applied compatibility patch to {modName}")
local logError = |modName, error| -> sb.logError($"[xSB] Error while applying compatibility patch for {modName}: {error}")
local modName

--- Compatibility patch for navigation UI mods ---

local morePlanetInfoPatch = jarray{
    jobject{
        op = "test",
        path = "/weatherThreatValues"
    },
    jobject{
        op = "add",
        path = "/weatherThreatValues/clear",
        value = 0.0
    }
}
-- The patch must be done post-load to «catch» the existence of More Planet Info and other navigation UI mods.
assets.add("/interface/cockpit/cockpit.config.postproc", morePlanetInfoPatch)
assets.patch("/interface/cockpit/cockpit.config", "/interface/cockpit/cockpit.config.postproc")

--- Compatibility patch for Patman's Infiniter Inventory ---

-- FezzedOne: This patch is needed to replace the unsafe smuggled Infiniter Inventory pane callback with a pane message,
-- since xStarbound does not allow unsafe callback smuggling for good reasons. Also includes forward compatibility for
-- any future usage of pane or widget callbacks on the stock inventory pane. The engine call for shift-clicking items
-- from the stock inventory is not currently implemented in xStarbound, so that feature isn't supported.

if not "pat_infinv" in loadedMods then goto skipInfiniterInventoryPatch end
modName = "Patman's Infiniter Inventory"

pluto_try
    local infiniterInventoryPlayerScriptPath = "/pat/infinv/binds/player.lua"
    local infiniterInventoryPlayerScript = assets.bytes(infiniterInventoryPlayerScriptPath)
    local infiniterInventoryPlayerScriptPatch = [==[
        local dismissFunc = function()
            world.sendEntityMessage(player.id(), "Patman::InfiniterInventory::dismiss")
        end
        shared.pat_infinv_dismiss = dismissFunc

        local patman_oldOpen = open
        function open(...)
            local ret = patman_oldOpen(...)
            -- The dismiss entry got cleared. Reset it.
            shared.pat_infinv_dismiss = dismissFunc
            return ret
        end
    ]==]
    assets.add(infiniterInventoryPlayerScriptPath, infiniterInventoryPlayerScript .. infiniterInventoryPlayerScriptPatch)

    local infiniterInventoryPaneScriptPath = "/pat/infinv/InfiniteInventory.lua"
    local infiniterInventoryPaneScript = assets.bytes(infiniterInventoryPaneScriptPath)
    local infiniterInventoryPaneScriptPatch = [==[
        -- FezzedOne: Includes a full compatibility bridge for all possible inventory pane/widget binds (except imperfect `addWidget` compatibility)
        -- for forward compatibility with updates to Infiniter Inventory that might modify the stock inventory pane.

        local patman_oldInit = init

        local __generateCanvasWidgetBridge = function(canvasName)
            local canvasWidgetBridge = {}
            local canvasWidgetBridgeMetatable = {
                __canvasWidgetName = canvasName,
                __index = function(self, key)
                    local savedKey = key
                    return function(...)
                        return world.sendEntityMessage("Patman::canvasWidgetBridge", self.__canvasWidgetName, savedKey, ...):result()
                    end
                end
            }
            setmetatable(canvasWidgetBridge, canvasWidgetBridgeMetatable)
            return canvasWidgetBridge
        end
    
        local __generateWidgetBridge = function()
            local widgetBridge = {}
            local widgetBridgeMetatable = {
                __index = function(_, key)
                    local savedKey = key
                    if savedKey == "bindCanvas" then
                        return function(canvasName)
                            return __generateCanvasWidgetBridge(canvasName)
                        end
                    else
                        return function(...)
                            return world.sendEntityMessage("Patman::widgetBridge", savedKey, ...):result()
                        end
                    end
                end
            }
            setmetatable(widgetBridge, widgetBridgeMetatable)
            return widgetBridge
        end

        function init()
            message.setHandler("Patman::InfiniterInventory::dismiss", function (_, isLocal)
                pane.dismiss()
            end)
            interface.bindRegisteredPane = function(paneName)
                if string.lower(paneName) == "inventory" then
                    local paneBridge = {}
                    local paneBridgeMetatable = {
                        __index = function(_, key)
                            local savedKey = key
                            if savedKey == "toWidget" then
                                return function()
                                    return __generateWidgetBridge()
                                end
                            else
                                return function(...)
                                    return world.sendEntityMessage("Patman::paneBridge", savedKey, ...):result()
                                end
                            end
                        end
                    }
                    setmetatable(paneBridge, paneBridgeMetatable)
                    return paneBridge
                else
                    return nil
                end
            end
            patman_oldInit()
        end
    ]==]
    assets.add(infiniterInventoryPaneScriptPath, infiniterInventoryPaneScript .. infiniterInventoryPaneScriptPatch)

    local inventoryBridgeScript = [==[
        -- FezzedOne: Bridge script needed to emulate OpenStarbound's version of `interface.bindRegisteredPane`.

        local inventoryPaneCallbacks, inventoryWidgetCallbacks
        function init()
            inventoryPaneCallbacks = interface.bindRegisteredPane("Inventory")
            inventoryWidgetCallbacks = inventoryPaneCallbacks.toWidget()
            message.setHandler("Patman::canvasWidgetBridge", function (_, isLocal, canvasName, funcKey, ...)
                if isLocal and inventoryWidgetCallbacks then
                    local canvasCallbacks = inventoryWidgetCallbacks.bindCanvas(canvasName)
                    if canvasCallbacks then
                        return canvasCallbacks[funcKey](canvasCallbacks, ...)
                    else
                        return nil
                    end
                end
            end)
            message.setHandler("Patman::widgetBridge", function (_, isLocal, funcKey, ...)
                if isLocal and inventoryWidgetCallbacks then
                    return inventoryWidgetCallbacks[funcKey](...)
                end
            end)
            message.setHandler("Patman::paneBridge", function (_, isLocal, funcKey, ...)
                if isLocal and inventoryPaneCallbacks then
                    return inventoryPaneCallbacks[funcKey](...)
                end
            end)
        end
    ]==]
    local inventoryBridgeScriptPath = "/pat/xsb/inventory_bridge.lua"
    assets.add(inventoryBridgeScriptPath, inventoryBridgeScript)
    local clientConfig = jobject{
        universeScriptContexts = jobject{ 
            pat_infinv = jarray{ 
                inventoryBridgeScriptPath
            }
        }
    }
    assets.add("/client.config.xSB-postproc", clientConfig)
    assets.patch("/client.config", "/client.config.xSB-postproc")

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipInfiniterInventoryPatch::

--- Compatibility patch for Patman's Save Inventory Position ---

if not "pat_saveInventoryPosition" in loadedMods then goto skipInventoryResetPatch end
modName = "Patman's Save Inventory Position"

pluto_try
    local saveInventoryPositionScriptPath = "/pat_saveInventoryPosition.lua"
    local inventoryResetCommandScriptPath = "/scripts/patmanInventoryResetCommandHandler.lua"

    local saveInventoryPositionScript = assets.bytes(saveInventoryPositionScriptPath)

    local clientConfig = jobject{
        universeScriptContexts = jobject{ 
            pat_saveInventoryPosition = jarray{ 
                saveInventoryPositionScriptPath
            }
        }
    }
    assets.add("/client.config.xSB-postproc", clientConfig)
    assets.patch("/client.config", "/client.config.xSB-postproc")

    local playerConfig = jobject{
        genericScriptContexts = jobject{
            pat_saveInventoryPosition = inventoryResetCommandScriptPath
        }
    }
    assets.add("/player.config.xSB-postproc", playerConfig)
    assets.patch("/player.config", "/player.config.xSB-postproc")

    local saveInventoryPositionPatch = [==[
        -- Note: The patched mod script only works properly when `"safeScripts"` is *enabled*.

        local patman_oldInit = init

        local oldPrimaryUuid = nil
        InventoryPane = nil

        function init()
            -- Need to use a persistent reference on xSB so that a pane callback is accessible on `uninit`.
            InventoryPane = interface.bindRegisteredPane(PaneName)
            local old_bindRegisteredPane = interface.bindRegisteredPane
            interface.bindRegisteredPane = function(paneName)
                if paneName == "Inventory" then
                    return InventoryPane
                else
                    return old_bindRegisteredPane(paneName)
                end
            end

            -- The message table is not currently available in universe client scripts on xClient.
            message ??= {
                setHandler = function() end
            }
            patman_oldInit()
            script.setUpdateDelta(1)

            oldPrimaryUuid = world.primaryPlayerUuid()
        end

        local resetIdentifier <const> = "patman::resetInventoryPosition"

        function update(dt)
            if world.getGlobal(resetIdentifier) then
                interface.bindRegisteredPane(PaneName).setPosition({0, 0})
                world.setGlobal(resetIdentifier, null)
            end

            oldPrimaryUuid = primaryUuid
        end
    ]==] .. "\n"
    assets.add(saveInventoryPositionScriptPath, saveInventoryPositionScript .. saveInventoryPositionPatch)

    local inventoryResetCommandScript = [==[
        local resetIdentifier <const> = "patman::resetInventoryPosition"

        function init()
            message.setHandler("/resetinventoryposition", |_, isLocal| ->
                (isLocal and player.uniqueId() == world.primaryPlayerUuid())
                    ? (world.setGlobal(resetIdentifier, true) or "Reset inventory position.")
                    : "^red;Attempted to invoke ^orange;/resetinventoryposition^red; on a non-local or secondary player!^reset;")
        end
    ]==] .. "\n"
    assets.add(inventoryResetCommandScriptPath, inventoryResetCommandScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipInventoryResetPatch::

--- Compatibility patch for Silver Sokolova's Quick Commands! ---

if not "XRC_quickCommands" in loadedMods then goto skipQuickCommandsPatch end
modName = "Silver Sokolova's Quick Commands"

pluto_try
    local quickCommandsScriptPath = "/xrc_bindings.lua"

    local quickCommandsScript = assets.bytes(quickCommandsScriptPath)
    local quickCommandsScriptPatch = [==[
        local quickCommands_oldInit = init
        function init()
            local oldAssetJson = root.assetJson
            root.assetJson = function(path)
                -- Fool the mod script into thinking it's running on OpenStarbound.
                local result = oldAssetJson(path)
                if path == "/player.config:genericScriptContexts" then
                    result.OpenStarbound = true
                end
                return result
            end
            quickCommands_oldInit()
        end
    ]==] .. "\n"
    assets.add(quickCommandsScriptPath, quickCommandsScript .. quickCommandsScriptPatch)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipQuickCommandsPatch::

--- Compatibility patch for keybinds in Patman's Ruler ---

if not "pat_ruler" in loadedMods then goto skipRulerPatch end
modName = "Patman's Ruler"

pluto_try
    local rulerScriptPath = "/pat/ruler/ruler.lua"

    local rulerScript = assets.bytes(rulerScriptPath)
    local rulerScriptPatch = [==[
        local starExtensions = true -- Fool the mod script into thinking it's running on StarExtensions.
    ]==] .. "\n"
    assets.add(rulerScriptPath, rulerScriptPatch .. rulerScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipRulerPatch::

--- Compatibility patch for Emmaker's Limited Lives ---

if not "LimitedLives" in loadedMods then goto skipLimitedLivesPatch end
modName = "Emmaker's Limited Lives"

pluto_try
    local limitedLivesScriptPath = "/scripts/player/limitedLives.lua"

    local limitedLivesScript = assets.bytes(limitedLivesScriptPath)
    local limitedLivesScriptPatch = [==[
        local starExtensions = true -- Fool the mod script into thinking it's running on StarExtensions.
    ]==] .. "\n"
    assets.add(limitedLivesScriptPath, limitedLivesScriptPatch .. limitedLivesScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipLimitedLivesPatch::

--- Compatibility patch for Patman's Matter Manipulator Keybinds ---

if not "pat_mmbinds" in loadedMods then goto skipMmBindsPatch end
modName = "Patman's Matter Manipulator Keybinds"

pluto_try
    local mmBindsScriptPath = "/pat_mmbinds.lua"

    local mmBindsScript = assets.bytes(mmBindsScriptPath)
    local mmBindsScriptPatch =  "\n" .. [==[
        local mmBinds_oldInit = init
        function init()
            root.assetOrigin = root.assetSource -- Emulate the StarExtensions callback expected by this mod.
            mmBinds_oldInit()
        end
    ]==]
    assets.add(mmBindsScriptPath, mmBindsScript .. mmBindsScriptPatch)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipMmBindsPatch::

--- Compatibility patch for RingSpokes' Unde Venis ---

if not "UndeVenis" in loadedMods then goto skipUndeVenisPatch end
modName = "RingSpokes' Unde Venis"

pluto_try
    local undeVenisScriptPath = "/interface/undevenis/undevenis.lua"
    local undeVenisScript = assets.bytes(undeVenisScriptPath)

    local undeVenisScriptPatch = "\n" .. [==[
        ---<< ORIGINAL UNDE VENIS SCRIPT ENDS HERE >>---

        UNKNOWN = "<unknown>"

        function init()
            root.assetOrigin = root.assetSource
            root.assetSourcePaths = function(addMetadata)
                if addMetadata then
                    local assetSources = root.assetSources()
                    local returnValue = jobject{}
                    for assetSources as source do
                        returnValue[source] = root.assetSourceMetadata(source)
                          |> |x| -> type(x.name) == "string" ? x : rawset(x, "name", sb.printJson(x.name))
                          |> |x| -> type(x.friendlyName) == "string" ? x : rawset(x, "friendlyName", sb.printJson(x.friendlyName))
                    end
                    return returnValue
                else
                    return root.assetSources()
                end
            end
        end   
    ]==]
    assets.add(undeVenisScriptPath, undeVenisScript .. undeVenisScriptPatch)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end
::skipUndeVenisPatch::

--- Compatibility patch for James' Wardrobe Cumulative Patch ---

if not "[oSB] Wardrobe Cumulative Patch" in loadedMods then goto skipWardrobeCumulativePatch end
modName = "James' Wardrobe Cumulative Patch"

pluto_try
    local wardrobeScriptPath = "/wardrobe_postload.lua"
    local wardrobeScript = assets.bytes(wardrobeScriptPath)

    local wardrobeScriptPatch = [==[
        assets.sourcePaths = function(addMetadata)
            if addMetadata then
                local assetSources = assets.sources()
                local returnValue = jobject{}
                for assetSources as source do
                    returnValue[source] = assets.sourceMetadata(source)
                end
                return returnValue
            else
                return assets.sources()
            end
        end

        ---<< ORIGINAL WARDROBE PATCH SCRIPT BEGINS HERE >>---
    ]==] .. "\n"
    assets.add(wardrobeScriptPath .. ".frontload", wardrobeScriptPatch .. wardrobeScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end
::skipWardrobeCumulativePatch::
