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
        local patman_oldInit = init
        local patman_oldUninit = uninit
        function init()
            -- The message table is not currently available in universe client scripts on xClient.
            message ??= {
                setHandler = function() end
            }
            patman_oldInit()
            script.setUpdateDelta(1)
        end

        local setIdentifier <const> = "patman::setInventoryPosition"
        local saveIdentifier <const> = "patman::saveInventoryPosition"
        local resetIdentifier <const> = "patman::resetInventoryPosition"

        function update(dt)
            -- Make sure the inventory is restored to its saved position on player swaps.
            if world.getGlobal(saveIdentifier) then
                patman_oldUninit()
                world.setGlobal(saveIdentifier, null)
            end
            
            if world.getGlobal(setIdentifier) then
                patman_oldInit()
                script.setUpdateDelta(1)
                world.setGlobal(setIdentifier, null)
            end
            
            if world.getGlobal(resetIdentifier) then
                interface.bindRegisteredPane(PaneName).setPosition({0, 0})
                world.setGlobal(resetIdentifier, null)
            end
        end
    ]==]
    assets.add(saveInventoryPositionScriptPath, saveInventoryPositionScript .. saveInventoryPositionPatch)

    local inventoryResetCommandScript = [==[
        local setIdentifier <const> = "patman::setInventoryPosition"
        local saveIdentifier <const> = "patman::saveInventoryPosition"
        local resetIdentifier <const> = "patman::resetInventoryPosition"

        function init()
            message.setHandler("/resetinventoryposition", |_, isLocal| ->
                isLocal ? world.setGlobal(resetIdentifier, true) : nil)
            script.setUpdateDelta(0)

            world.setGlobal(setIdentifier, true)
        end

        function uninit()
            world.setGlobal(saveIdentifier, true)
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

    local rulerScript = assets.bytes(rulerScriptPath)
    local rulerScriptPatch = [==[
        local starExtensions = true -- Fool the mod script into thinking it's running on StarExtensions.
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

    local limitedLivesScript = assets.bytes(limitedLivesScriptPath)
    local limitedLivesScriptPatch = [==[
        local starExtensions = true -- Fool the mod script into thinking it's running on StarExtensions.
    ]==]
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
    local mmBindsScriptPatch = [==[
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