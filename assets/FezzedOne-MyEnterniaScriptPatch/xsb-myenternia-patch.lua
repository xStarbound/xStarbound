-- FezzedOne's My Enternia Patch for xStarbound --
local scriptPath = "/items/buildscripts/alta/object.lua"
if xsb and assets.exists(scriptPath) then
    local objectScript = assets.bytes(scriptPath)
    objectScript = objectScript:gsub('"%%02s"', '"%%2s"')
    assets.add(scriptPath, objectScript)
end

local scriptPath2 = "/items/buildscripts/alta/item.lua"
if assets.exists(scriptPath2) then
    local itemScript = assets.bytes(scriptPath2)
    itemScript = itemScript:gsub("if get%('objectImage'%) then", "if type(get('objectImage')) == 'string' then")
    assets.add(scriptPath2, itemScript)
end