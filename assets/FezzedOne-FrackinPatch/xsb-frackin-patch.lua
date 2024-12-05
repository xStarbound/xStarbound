-- FezzedOne's Frackin' Patch --
local needsPatch = "/scripts/fupower.lua"
local needsPatch2 = "/objects/crafting/clonelab/clonelab.lua"

local patchCode = [==[
function power.onNodeConnectionChange(arg, iterations)
    if power.objectPowerType then
        local inputCounter = 0
        local outputCounter = 0
        if power.objectPowerType == "battery" then return arg end
        --sb.logInfo("iterations: %s",iterations)
        iterations = (iterations and iterations + 1) or 1
        if arg then
            entitylist = arg
        else
            -- FezzedOne: Needs `jarray` because xStarbound's JSON coercion defaults to assuming tables are objects
            -- and this table is passed back and forth through `world.callScriptedEntity`, which means multiple coercions
            -- (and conversion of integer keys to strings, breaking iteration in this script).
            if power.objectPowerType == "battery" then
                entitylist =
                    jobject({ battery = jarray({ entity.id() }), output = jarray({}), all = jarray({ entity.id() }) })
            elseif power.objectPowerType == "output" then
                entitylist =
                    jobject({ battery = jarray({}), output = jarray({ entity.id() }), all = jarray({ entity.id() }) })
            else
                entitylist = jobject({ battery = jarray({}), output = jarray({}), all = jarray({ entity.id() }) })
            end
        end
        if iterations < 100 then
            for i = 0, object.inputNodeCount() - 1 do
                if object.isInputNodeConnected(i) then
                    local idlist = object.getInputNodeIds(i)
                    inputCounter = inputCounter + util.tableSize(idlist)
                    for value in pairs(idlist) do
                        powertype = callEntity(value, "isPower", iterations)
                        if powertype then
                            for j = 1, #entitylist.all + 1 do
                                if j == #entitylist.all + 1 then
                                    if powertype == "battery" then
                                        table.insert(entitylist.battery, value)
                                    elseif powertype == "output" then
                                        table.insert(entitylist.output, value)
                                    end
                                    table.insert(entitylist.all, value)
                                    entitylist = (
                                        callEntity(value, "power.onNodeConnectionChange", entitylist, iterations)
                                        or entitylist
                                    )
                                elseif entitylist.all[j] == value then
                                    break
                                end
                            end
                        end
                    end
                end
            end
            for i = 0, object.outputNodeCount() - 1 do
                if object.isOutputNodeConnected(i) then
                    local idlist = object.getOutputNodeIds(i)
                    outputCounter = outputCounter + util.tableSize(idlist)
                    for value in pairs(idlist) do
                        powertype = callEntity(value, "isPower", iterations)
                        if powertype then
                            for j = 1, #entitylist.all + 1 do
                                if j == #entitylist.all + 1 then
                                    if powertype == "battery" then
                                        table.insert(entitylist.battery, value)
                                    elseif powertype == "output" then
                                        table.insert(entitylist.output, value)
                                    end
                                    table.insert(entitylist.all, value)
                                    entitylist = (
                                        callEntity(value, "power.onNodeConnectionChange", entitylist, iterations)
                                        or entitylist
                                    )
                                elseif entitylist.all[j] == value then
                                    break
                                end
                            end
                        end
                    end
                end
            end
        else
            sb.logWarn("fupower.lua:power.onNodeConnectionChange: Critical recursion threshhold reached!")
        end
        if arg then
            return entitylist
        else
            power.entitylist = entitylist
            for i = 2, #entitylist.all do
                callEntity(entitylist.all[i], "updateList", entitylist)
            end
        end
        power.lastInputCount = inputCounter
        power.lastOutputCount = outputCounter
    end
end
]==]

local patchCode2 = [==[
null = nil -- The script in question expects `null` to be exactly the same as `nil` to avoid a nil dereference error.
-- This conflicts with xStarbound's special `null` value. Fix your shit, Sayter!
]==]

if xsb then 
    if assets.exists(needsPatch) then
        local patchedScript = assets.bytes(needsPatch) .. patchCode
        assets.add(needsPatch, patchedScript)
    end
    if assets.exists(needsPatch2) then
        local patchedScript2 = assets.bytes(needsPatch2) .. patchCode2
        assets.add(needsPatch2, patchedScript2)
    end
end
