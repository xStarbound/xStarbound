--- xStarbound StarExtensions compatibility handler ---

starExtensions = {
    version = function()
        if xsb then
            return "xStarbound v" .. xsb.version
        else
            return "compat"
        end
    end,
}

local base_getmetatable = getmetatable
local smuggledHandlersToCall = {}
local compatVarName = "SCCCompat::SmuggledMessages"
local soundCompatVarName = "SCCCompat::SmuggledSoundMessage"

local function jsonPack(...)
    local packedArgs = table.pack(...)
    local argNum = packedArgs.n
    packedArgs.n = nil
    if argNum > 0 then
        for i = 1, argNum do
            if packedArgs[argNum] == nil then packedArgs[argNum] = null end
        end
        return jarray(packedArgs)
    else
        return jarray()
    end
end

local function jsonUnpack(argArr, i)
    i = i or 1
    local argNum = jsize(argArr)
    if i <= argNum then return argArr[i], jsonUnpack(argArr, i + 1) end
end

local function handleSharedTable()
    local newSharedTable = {}
    local newSharedMetatable = {
        __index = function(t, key)
            if xsb and world then
                if key == "setMessageHandler" then
                    return function(handledMessage, handler) -- Need to replace this call with a safe version for StarCustomChat.
                        if message then
                            message.setHandler(handledMessage, handler)
                        elseif xsb and world then
                            if type(handledMessage) == "string" then -- To simplify logic, only handle strings in `message`.
                                if handledMessage == "icc_request_player_portrait" or handledMessage == "icc_is_chat_open" then
                                    return
                                end
                                local smuggles = world.getGlobal(compatVarName) or jobject({})
                                smuggles[handledMessage] = jarray()
                                table.insert(smuggles[handledMessage], jobject())
                                -- {
                                    -- `-1` for a message with no real handler set up yet;
                                    -- `0` for a message with a handler set up, but no messages received;
                                    -- an incremented number for each received message.
                                --     heartbeat = -1,
                                --     isLocal = null,
                                --     contents = null,
                                -- }
                                world.setGlobal(compatVarName, smuggles)
                                -- Add a fake handler to the smuggling list.
                                smuggledHandlersToCall[handledMessage] = {
                                    heartbeat = 0,
                                    handler = handler,
                                }
                            end
                        end
                    end
                elseif key == "sccTalkingSound" then
                    if status.getPersistentEffects("scctalking")[1] then -- Need to emulate SCC's status effect detection.
                        return function(soundData) world.setGlobal(soundCompatVarName, soundData) end
                    else
                        return nil
                    end
                else
                    local globals = world.getGlobals() or jobject({})
                    return globals[key]
                end
            else
                return rawget(t, key)
            end
        end,
        __newindex = function(t, key, value)
            if xsb and world then
                if key == "setMessageHandler" or key == "sccTalkingSound" then -- Need to fake this for StarCustomChat.
                    if type(value) ~= "function" then -- Don't do anything if the value is a function.
                        rawset(t, key, value)
                    end
                else
                    local globals = world.getGlobals() or jobject({})
                    globals[key] = value
                    world.setGlobals(globals)
                end
            else
                rawset(t, key, value)
            end
        end,
    }
    setmetatable(newSharedTable, newSharedMetatable)
    return newSharedTable
end

function getmetatable(value) -- Replaced with a safe, xStarbound-compatible wrapper.
    if type(value) ~= "table" then
        local newGlobalTable = {}
        local newGlobalMetatable = {
            __index = function(t, key)
                if xsb and world then
                    if key == "shared" then
                        return handleSharedTable()
                    else
                        local globals = world.getGlobals() or jobject({})
                        return globals[key]
                    end
                else
                    return rawget(t, key)
                end
            end,
            __newindex = function(t, key, value)
                if xsb and world then
                    if key == "shared" then -- Need to fake this for StarCustomChat.
                        -- Don't do anything here.
                    else
                        local globals = world.getGlobals() or jobject({})
                        globals[key] = value
                        world.setGlobals(globals)
                    end
                else
                    rawset(t, key, value)
                end
            end,
        }
        setmetatable(newGlobalTable, newGlobalMetatable)
        return newGlobalTable
    else
        return base_getmetatable(value)
    end
end

local function handleSmuggledHandlers()
    if xsb and world then
        local globalSmuggles = world.getGlobal(compatVarName)
        for handlerName, handledMessages in pairs(globalSmuggles or jobject({})) do
            if message and not smuggledHandlersToCall[handlerName] then
                if handledMessages then
                    message.setHandler(handlerName, function(innerName, isLocal, ...)
                        if xsb and world then
                            local smuggles = world.getGlobal(compatVarName) or jobject()
                            if smuggles[innerName] then
                                local msgCount = #smuggles[innerName]
                                local heartbeat = msgCount ~= 0 and smuggles[innerName][msgCount].heartbeat or 0
                                table.insert(smuggles[innerName], {
                                    heartbeat = heartbeat + 1,
                                    isLocal = isLocal,
                                    contents = jsonPack(...),
                                })
                            end
                            world.setGlobal(compatVarName, smuggles)
                        end
                    end)
                end
            elseif smuggledHandlersToCall[handlerName] then
                local handler = smuggledHandlersToCall[handlerName]
                
                if handledMessages then
                    local newMessageArray = jarray()
                    for _, msg in ipairs(handledMessages) do
                        if
                            msg.heartbeat
                            and handler.handler
                            and msg.heartbeat > handler.heartbeat
                            and msg.contents
                        then
                            handler.handler(handlerName, msg.isLocal, jsonUnpack(msg.contents))
                            handler.heartbeat = msg.heartbeat
                        else
                            table.insert(newMessageArray, msg)
                        end
                    end
                    globalSmuggles[handlerName] = newMessageArray
                end
            end
        end
        world.setGlobal(compatVarName, globalSmuggles)
    end
end

-- Wrap any attempts to set the `init`, `update` or `uninit` functions to make sure fake handlers are handled.

-- local envMetatable = base_getmetatable(_ENV)
-- rawset(envMetatable or {}, "__newindex", function(t, key, value)
--     if (key == "init" or key == "update" or key == "uninit") and type(value) == "function" then
--         local wrapperFunc = function(...)
--             local wrappedFunc = value
--             handleSmuggledHandlers()
--             return wrappedFunc(...)
--         end
--         rawset(t, key, wrapperFunc)
--     else
--         rawset(t, key, value)
--     end
-- end)
-- setmetatable(_ENV, envMetatable)

-- < START OF BASE SCRIPT >
