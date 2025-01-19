--- NPC Mechs patch for xStarbound ---

-- This patch fixes a typo in one of NPC Mechs' scripts that causes a compatibility issue with xStarbound.

local modularMechNpc2ScriptPath = "/vehicles/modularmech/modularmechnpc2.lua"

if xsb and assets.exists(modularMechNpc2ScriptPath) then
    local typoLine = "if thisarm.npcfiretime > 0then%-%- %-maxtyme then"
    local patchedLine = "if thisarm.npcfiretime > 0 then -- -maxtyme then"
    local scriptToPatch = assets.bytes(modularMechNpc2ScriptPath)
    assets.remove(modularMechNpc2ScriptPath)
    assets.add(modularMechNpc2ScriptPath, scriptToPatch:gsub(typoLine, patchedLine))
end