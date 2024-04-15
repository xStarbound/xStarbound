require("/scripts/util.lua")

local commands = {}

local function checkHostType(cid)
    if universe.isLocal(cid) then
        return "ownClient"
    else
        local connectionIds = universe.clientIds()
        for _, clientId in ipairs(connectionIds) do
            if universe.isLocal(clientId) then return "otherClient" end
        end
        return "server"
    end
end

local function executeLuaSource(cid, args)
    if universe.isAdmin(cid) then
        if load then
            local src = args[1]
            local success, result = pcall(load, src, "/run", "t", _ENV)
            if not success then
                return "^#f00;Compiler error: " .. result
            else
                local success, result = pcall(result)
                if not success then
                    return "^#f00;Error: " .. result
                else
                    local success, printed = pcall(sb.printJson, result)
                    if not success then
                        success, printed = pcall(sb.print, result)
                    end
                    if not success then
                        return "^#f00;Could not print return value: " .. printed
                    else
                        return printed
                    end
                end
            end
        else
            return '^cyan;"safeScripts"^reset; must be disabled in ^cyan;'
                .. (checkHostType(cid) == "server" and "xserver.config" or "xclient.config")
                .. "^reset; in order to use this command."
        end
    else
        return "Insufficient privileges to execute Lua code on the server"
    end
end

commands.serverrun = executeLuaSource
commands.serverexec = executeLuaSource

function commands.xserver(cid, _)
    local hostType = checkHostType(cid)
    local multiplayer = #universe.clientIds() ~= 1
    if hostType == "ownClient" then
        if multiplayer then
            return "^#f33;<xSB-2::xClient v" .. xsb.version() .. " as host>^reset;"
        else
            return "^#f33;<xSB-2::xClient v" .. xsb.version() .. ">^reset;"
        end
    elseif hostType == "otherClient" then
        return "^#f33;<hosted on xSB-2::xClient v" .. xsb.version() .. ">^reset;"
    else
        return "^#f33;<xSB-2::xServer v" .. xsb.version() .. ">^reset;"
    end
end

function commands.who(cid, _)
    local hostType = checkHostType(cid)
    local connectionIds = universe.clientIds()
    local playerCount = #connectionIds
    if hostType == "ownClient" and playerCount == 1 then return "Client is in single-player" end
    local outputString = "^#f33;"
        .. tostring(playerCount)
        .. (playerCount == 1 and " player online: " or " players online: ")
        .. "^reset;\n"
    if playerCount ~= 0 then
        for _, client in ipairs(connectionIds) do
            local nick = universe.clientNick(client)
            if nick == "" then
                nick = "^#f33;<$" .. toString(client) .. ">^reset;"
            else
                nick = nick .. "^reset;"
            end
            outputString = outputString .. nick .. ", "
        end
        outputString = outputString:sub(1, -3)
    else
        outputString = "No players online"
    end
    return outputString
end

function commands.world(cid, _)
    local hostType = checkHostType(cid)
    local connectionIds = universe.clientIds()
    local universePlayerCount = #connectionIds
    if hostType == "ownClient" and universePlayerCount == 1 then return "Client is in single-player" end
    local worldConnectionIds = {}
    local worldId = universe.clientWorld(cid)
    for _, connectionId in ipairs(connectionIds) do
        if universe.clientWorld(connectionId) == worldId then table.insert(worldConnectionIds, connectionId) end
    end
    local playerCount = #worldConnectionIds
    local outputString = "^#f33;"
        .. tostring(playerCount)
        .. (playerCount == 1 and " player on this world: " or " players on this world: ")
        .. "^reset;\n"
    if playerCount ~= 0 then
        for _, client in ipairs(worldConnectionIds) do
            local nick = universe.clientNick(client)
            if nick == "" then
                nick = "^#f33;<$" .. tostring(client) .. ">^reset;"
            else
                nick = nick .. "^reset;"
            end
            outputString = outputString .. nick .. ", "
        end
        outputString = outputString:sub(1, -3)
    else
        outputString = "No players on this world"
    end
    return outputString
end

function commands.listworlds(cid, _)
    if universe.isAdmin(cid) then
        local activeWorlds = universe.activeWorlds()
        local worldCount = #activeWorlds
        if worldCount ~= 0 then
            local listing = "^#f33;"
                .. tostring(worldCount)
                .. " active "
                .. (worldCount == 1 and "world" or "worlds")
                .. ":^reset;\n"
            for n, worldId in ipairs(activeWorlds) do
                listing = listing .. worldId .. (n == worldCount and "" or "\n")
            end
            return listing
        else
            return "No active worlds"
        end
    else
        return "Insufficient privileges to list active worlds"
    end
end

function commands.listworld(cid, args)
    if universe.isAdmin(cid) then
        local hostType = checkHostType(cid)
        local connectionIds = universe.clientIds()
        local worldConnectionIds = {}
        local worldId = args[1] or universe.clientWorld(cid) 
        if worldId then
            for _, connectionId in ipairs(connectionIds) do
                if universe.clientWorld(connectionId) == worldId then
                    table.insert(worldConnectionIds, connectionId)
                end
            end
            local playerCount = #worldConnectionIds
            if playerCount ~= 0 then
                local listing = "^#f33;"
                    .. tostring(playerCount)
                    .. (playerCount == 1 and " player" or " players")
                    .. " on world '"
                    .. worldId
                    .. "':^reset;\n"
                for n, connectionId in ipairs(worldConnectionIds) do
                    listing = listing
                        .. "$"
                        .. connectionId
                        .. " : "
                        .. universe.clientNick(connectionId)
                        .. " : $$"
                        .. universe.clientUuid(connectionId)
                        .. (n == playerCount and "" or "\n")
                end
                return listing
            else
                return "No players on world '" .. worldId .. "'"
            end
        else
            return "Must specify a valid world ID."
        end
    else
        return "Insufficient privileges to list players"
    end
end

function command(name, cid, args)
    if type(commands[name]) == "function" then
        return commands[name](cid, args)
    else
        return "Command ^orange;/" .. name .. "^reset; does not exist."
    end
end
