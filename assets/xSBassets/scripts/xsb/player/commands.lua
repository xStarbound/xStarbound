local module = {}
modules.commands = module

local commands = {}
local function command(name, func) commands[name] = func end

function module.init()
    for name, func in pairs(commands) do
        message.setHandler({ name = "/" .. name, localOnly = true }, func)
    end
end

local function executeLuaSource(src)
    if load then
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
        return '^cyan;"safeScripts"^reset; must be disabled in ^cyan;xclient.config^reset; in order to use this command.'
    end
end

command("run", executeLuaSource)
command("exec", executeLuaSource)

local function xsbGuide(subCommand)
    if subCommand == "" then
        local guideTextLines = {
            "^#f55;xSB-2 xClient v^red;" .. xsb.version() .. "^reset;",
            "Run ^red;/xsb^reset; with one of the following for command help:",
            "^red;character^reset;, ^red;voice^reset;, ^red;server^reset;, ^red;misc^reset;",
        }
        local guideText = ""
        for n, line in ipairs(guideTextLines) do
            guideText = guideText .. line .. (n < #guideTextLines and "\n" or "")
        end
        return guideText
    elseif subCommand == "character" then
        return [===[^white;Character commands^reset;
Swap character by name: ^red;/swap [name or part of name]^reset;
Swap character by UUID: ^red;/swapuuid [UUID]^reset;
Edit humanoid identity: ^red;/identity^reset; (run for subcommands)
View description: ^red;/description^reset; (nothing after the command)
Edit description: ^red;/description [new description in quotes]^reset;]===]
    elseif subCommand == "voice" then
        return [===[^white;Voice commands^reset;
Mute speaker by name: ^red;/mute [name or part of name]^reset;
Unmute speaker by name: ^red;/unmute [name of part of name]^reset;
Mute speaker by UUID: ^red;/muteuuid [UUID]^reset;
Unmute speaker by UUID: ^red;/unmuteuuid [UUID]^reset;
List muted speakers: ^red;/listmutes^reset;]===]
    elseif subCommand == "server" then
        return [===[^white;Server commands^reset;
These commands require xServer or (on a Steam or Discord host) xClient.
List all players on the server: ^red;/who^reset;
List players on the same world: ^red;/world^reset;]===]
    elseif subCommand == "misc" then
        return [===[^white;Miscellaneous commands^reset;
Export generated directive sprites as PNGs: ^red;/render^reset; (run for subcommands)]===]
    end
end

command("xsb", xsbGuide)

local function handleIdentity(rawArgs)
    if rawArgs == "" then
        local helpMessage = [===[]===]
        return helpMessage
    else
        local args = {}
        for arg in rawArgs:gmatch("%S+") do
            table.insert(args, arg)
        end
        if args[1] == "set" then
            if not args[2] then
                local setHelpMessage = [===[]===]
                return setHelpMessage
            elseif args[2] == "species" then
                if args[3] then
                    player.setSpecies(args[3])
                    return "Attempted to set species to ^red;'" .. args[3] .. "'^reset;."
                else
                    return "Must specify a species."
                end
            elseif args[2] == "gender" then
                if args[3] then
                    player.setGender(args[3])
                    return "Attempted to set gender to ^red;'" .. args[3] .. "'^reset;."
                else
                    return "Must specify a gender."
                end
            elseif args[2] == "" then
            end
        elseif args[1] == "get" then
        else
        end
    end
end

command("identity", handleIdentity)
