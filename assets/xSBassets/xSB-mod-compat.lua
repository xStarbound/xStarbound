local loadedModPaths = assets.loadedSources
local loadedMods = {}
for loadedModPaths as loadedMod do
    table.insert(loadedMods, assets.sourceMetadata(loadedMod).name ?? loadedMod)
end

local logInfo = |modName| -> sb.logError($"[xSB] Detected and applied compatibility patch to {modName}")
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

--- Compatibility patch for Patman's Save Inventory Position ---

if not "pat_saveInventoryPosition" in loadedMods then goto skipInventoryResetPatch end
modName = "Patman's Save Inventory Position"

pluto_try
    local saveInventoryPositionScriptPath = "/pat_saveInventoryPosition.lua"
    local inventoryResetCommandScriptPath = "/scripts/patmanInventoryResetCommandHandler.lua"

    local playerConfig = assets.json("/player.config")
    local clientConfig = assets.json("/client.config")
    local saveInventoryPositionScript = assets.bytes(saveInventoryPositionScriptPath)

    clientConfig.universeScriptContexts.pat_saveInventoryPosition = jarray{ saveInventoryPositionScriptPath }
    assets.add("/client.config", clientConfig)

    playerConfig.genericScriptContexts.pat_saveInventoryPosition = inventoryResetCommandScriptPath
    assets.add("/player.config", playerConfig)

    local saveInventoryPositionPatch = [==[
        local oldInit = init
        function init()
            -- The message table is not currently available in universe client scripts on xClient.
            message ??= {
                setHandler = function() end
            }
            oldInit()
            script.setUpdateDelta(1)
        end

        local inventoryPositionIdentifier <const> = "patman::inventoryPosition"

        function update(dt)
            if inventoryPosition := world.getGlobal(inventoryPositionIdentifier) then
                inv?.setPosition({0, 0})
                world.setGlobal(inventoryPositionIdentifier, null)
            end
        end
    ]==]
    assets.add(saveInventoryPositionScriptPath, saveInventoryPositionScript .. saveInventoryPositionPatch)

    local inventoryResetCommandScript = [==[
        function init()
            local inventoryPositionIdentifier <const> = "patman::inventoryPosition"

            message.setHandler("/resetinventoryposition", |_, isLocal| ->
                isLocal ? world.setGlobal(inventoryPositionIdentifier, true) : nil)
            script.setUpdateDelta(0)
        end
    ]==]
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
    local quickCommandsScriptPath = "/pat_dumpids.lua"

    local quickCommandsScript = assets.bytes(quickCommandsScriptPath)
    local quickCommandsScriptPatch = [==[
        local oldInit = init
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
            oldInit()
        end
    ]==]
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

    local rulerScript = assets.bytes(rulerScript)
    local rulerScriptPatch = [==[
        local starExtensions = true -- Fool Patman's Ruler into thinking it's running on StarExtensions.
    ]==]
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

    local limitedLivesScript = assets.bytes(limitedLivesScript)
    local limitedLivesScriptPatch = [==[
        local starExtensions = true -- Fool Emmaker's Limited Lives into thinking it's running on StarExtensions.
    ]==]
    assets.add(limitedLivesScriptPath, limitedLivesScriptPatch .. limitedLivesScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end

::skipLimitedLivesPatch::