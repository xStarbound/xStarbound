-- FezzedOne's My Enternia Patch for xStarbound --
local scriptPath = "/items/buildscripts/alta/object.lua"
if xsb and assets.exists(scriptPath) then
  local objectScript = assets.bytes(scriptPath)
  objectScript = objectScript:gsub('"%%02s"', '"%%2s"')
  assets.add(scriptPath, objectScript)
end