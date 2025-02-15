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

        function init()
            if not root.getConfiguration("safeScripts") then
                interface.addChatMessage({
                    message = "^#f22;[xSB]^reset; You must enable ^cyan;\"safeScripts\"^reset; in ^cyan;xclient.config^reset; " .. 
                    "to use Patman's Save Inventory Position with xClient.",
                    mode = "CommandResult"
                })
                update = nil
                return
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
            if not root.getConfiguration("safeScripts") then return end
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

    local undeVenisScriptPatch = [==[
        function init()
            root.assetOrigin = root.assetSource
            root.assetSourcePaths = function(addMetadata)
                if addMetadata then
                    local assetSources = root.assetSources()
                    local returnValue = jobject{}
                    for assetSources as source do
                        returnValue[source] = root.assetSourceMetadata(source)
                    end
                    return returnValue
                else
                    return root.assetSources()
                end
            end
        end

        ---<< ORIGINAL UNDE VENIS SCRIPT BEGINS HERE >>---
    ]==] .. "\n"
    assets.add(undeVenisScriptPath, undeVenisScriptPatch .. undeVenisScript)

    logInfo(modName)
pluto_catch e then
    logError(modName, e)
end
::skipUndeVenisPatch::