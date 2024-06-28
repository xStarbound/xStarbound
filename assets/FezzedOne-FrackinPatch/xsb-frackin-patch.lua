-- FezzedOne's Frackin' Patch --
local needsPatch = "/frackinship/scripts/quest/shipupgrades.lua"

local patchCode = [==[
function calculateNew(stat, modifier, oldModifier, currentAmount)
	local info = upgradeConfig[stat]
	if info then
		statModifier = math.max(modifier, type(info.min) == "number" and info.min or -math.huge)
		--sb.logInfo("1. " .. statModifier)
		statModifier = math.min(statModifier, type(info.max) == "number" and info.max or math.huge)
		--sb.logInfo("2. " .. statModifier)
		statNew = math.max(currentAmount + (statModifier - oldModifier), info.totalMin or -math.huge)
		--sb.logInfo("3. " .. statNew)
		statNew = math.min(statNew, info.totalMax or math.huge)
		--sb.logInfo("4. " .. statNew)
		return statNew, statModifier
	end
end
]==]

if xsb and assets.exists(needsPatch) then
  local patchedScript = assets.bytes(needsPatch) .. patchCode
  assets.add(needsPatch, patchedScript)
end

