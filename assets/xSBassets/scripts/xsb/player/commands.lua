local module = {}
modules.commands = module

local commands = {}
local function command(name, func) commands[name] = func end

function module:init()
    for name, func in pairs(commands) do
        message.setHandler({ name = "/" .. name, localOnly = true }, func)
    end
    self.muteCheckTimer = 0.0
    local showAllSpeakers = root.getConfiguration("voiceShowAllSpeakers")
    if showAllSpeakers == nil then
        showAllSpeakers = false
        root.setConfiguration("voiceShowAllSpeakers", false)
    end
    math.__showAllSpeakers = showAllSpeakers
    self.muteCheckInterval = root.getConfiguration("voiceMuteCheckInterval")
    if (not self.muteCheckInterval) or (type(self.muteCheckInterval) == "number" and self.muteCheckInterval < 0) then
        root.setConfiguration("voiceMuteCheckInterval", 1.0)
    end
    self.muteCheckInterval = self.muteCheckInterval or 1.0
    self.oldMutes = jobject()
end

-- /run and /exec --

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

-- /xsb --

local function xsbGuide(subCommand)
    if subCommand == "" then
        local guideTextLines = {
            "^#f33;<xSB-2::xClient v" .. xsb.version() .. ">^reset;",
            "Run ^cyan;/xsb^reset; with one of the following for command help:",
            "^cyan;character^reset;, ^cyan;voice^reset;, ^cyan;server^reset;, ^cyan;misc^reset;",
        }
        local guideText = ""
        for n, line in ipairs(guideTextLines) do
            guideText = guideText .. line .. (n < #guideTextLines and "\n" or "")
        end
        return guideText
    elseif subCommand == "character" then
        return [===[^#f33;<character commands>^reset;
Swap character by name: ^cyan;/swap [name or part of name]^reset;
Swap character by UUID: ^cyan;/swapuuid [UUID]^reset;
Edit humanoid identity: ^cyan;/identity^reset; (run for subcommands)
View character's game mode: ^cyan;/gamemode^reset; (nothing after the command)
Change character's game mode: ^cyan;/gamemode [casual/survival/hardcore]^reset; (requires ^cyan;/admin^reset;)
View character description: ^cyan;/description^reset; (nothing after the command)
Edit character description: ^cyan;/description [new description in quotes]^reset;]===]
    elseif subCommand == "voice" then
        return [===[^#f33;<voice commands>^reset;
Mute speaker by name: ^cyan;/mute [name or part of name]^reset;
Unmute speaker by name: ^cyan;/unmute [name of part of name]^reset;
List muted speakers: ^cyan;/listmutes^reset;
Adjust voice settings: ^cyan;/voice [subcommand]^reset; (invoke without subcommand for help)]===]
    elseif subCommand == "server" then
        return [===[^#f33;<server commands>^reset;
These commands require xServer or xClient on the server/host.
Check xClient/xServer version on server/host: ^cyan;/xserver^reset;
List all players on the server: ^cyan;/who^reset;
List players on the same world as you: ^cyan;/world^reset;]===]
    elseif subCommand == "misc" then
        return [===[^#f33;<misc commands>^reset;
Export generated directive sprites as PNGs: ^cyan;/render^reset; (run for subcommands)
Run client-side Lua code: ^cyan;/run^reset; (^cyan;"safeScripts"^reset; must be disabled on client)]===]
    end
end

command("xsb", xsbGuide)

-- /identity --

local identityKeys = {
    "bodyDirectives",
    "color",
    "emoteDirectives",
    "facialHairDirectives",
    "facialHairGroup",
    "facialHairType",
    "facialMaskDirectives",
    "facialMaskGroup",
    "facialMaskType",
    "gender",
    "hairDirectives",
    "hairGroup",
    "hairType",
    "imagePath",
    "name",
    "personalityArmIdle",
    "personalityArmOffset",
    "personalityHeadOffset",
    "personalityIdle",
    "species",
}

local identitySetCommands = {
    "bodyDirectives [string]",
    "color [r] [g] [b]",
    "emoteDirectives [string]",
    "facialHairDirectives [string]",
    "facialHairGroup [string]",
    "facialHairType [string]",
    "facialMaskDirectives [string]",
    "facialMaskGroup [string]",
    "facialMaskType [string]",
    "gender [male/female]",
    "hairDirectives [string]",
    "hairGroup [string]",
    "hairType [string]",
    "imagePath <string>",
    "name [name]",
    "personalityArmIdle [string]",
    "personalityArmOffset [x] [y]",
    "personalityHeadOffset [x] [y]",
    "personalityIdle [string]",
    "species [string]",
}

local function getIdentitySetCommands()
    local returnString = ""
    for n, subcommand in ipairs(identitySetCommands) do
        returnString = returnString
            .. "^cyan;/identity set "
            .. subcommand
            .. "^reset;"
            .. (n < #identitySetCommands and "\n" or "")
    end
    return returnString
end

local function getIdentityGetCommands()
    local returnString = ""
    for n, subcommand in ipairs(identityKeys) do
        returnString = returnString
            .. "^cyan;/identity get "
            .. subcommand
            .. "^reset;"
            .. (n < #identityKeys and "\n" or "")
    end
    return returnString
end

local function handleIdentity(rawArgs)
    if rawArgs == "" then
        local helpMessage = [===[^#f33;</identity>^reset;
Get an identity key: ^cyan;/identity get [subcommand]^reset; (see ^cyan;/identity get^reset; for subcommands)
Set an identity key: ^cyan;/identity set [subcommand]^reset; (see ^cyan;/identity set^reset; for subcommands)

Some common subcommands:
Set your name: ^cyan;/identity set name [newName]^reset;
Set your species: ^cyan;/identity set species [newSpecies]^reset;
]===]
        return helpMessage
    else
        local args = {}
        for arg in rawArgs:gmatch("%S+") do
            table.insert(args, arg)
        end
        if args[1] == "set" then
            if not args[2] then
                local setHelpMessage = "^#f33;</identity set>^reset;\nAvailable keys:\n"
                    .. getIdentitySetCommands()
                    .. "\nNot specifying ^cyan;<string>^reset; for ^cyan;imagePath^reset; sets it to ^cyan;null^reset;."
                    .. "\nFor ^cyan;name^reset;, the new name must be quoted or escaped if it contains any spaces."
                return setHelpMessage
            else
                local key = args[2]
                if key == "personalityArmOffset" or key == "personalityHeadOffset" then
                    local x, y = tonumber(args[3]), tonumber(args[4])
                    if x and y then
                        player.setIdentity({
                            [key] = { x, y },
                        })
                        return "Set ^cyan;" .. key .. "^reset; to ^cyan;[" .. x .. ", " .. y .. "]^reset;."
                    else
                        return "Missing or invalid arguments. Syntax is ^cyan;/set "
                            .. key
                            .. " [x] [y]^reset;,"
                            .. " where ^cyan;x^reset; and ^cyan;y^reset; are numbers specifying a pixel offset."
                    end
                elseif key == "color" then
                    local r, g, b = tonumber(args[3]), tonumber(args[4]), tonumber(args[5])
                    if r and g and b then
                        if r >= 0 and g >= 0 and b >= 0 and r <= 255 and g <= 255 and b <= 255 then
                            r, g, b = math.floor(r), math.floor(g), math.floor(b)
                            player.setIdentity({
                                color = { r, g, b },
                            })
                            return "Set ^cyan;color^reset; to ^cyan;[" .. r .. ", " .. g .. ", " .. b .. "]^reset;."
                        else
                            return "Missing or invalid arguments. Syntax is ^cyan;/set color [r] [g] [b]^reset;,"
                                .. " where ^cyan;r^reset;, ^cyan;g^reset; and ^cyan;b^reset; are numbers between"
                                .. " ^cyan;0^reset; and ^cyan;255^reset;, inclusive."
                        end
                    else
                        return "Missing or invalid arguments. Syntax is ^cyan;/set color [r] [g] [b]^reset;,"
                            .. " where ^cyan;r^reset;, ^cyan;g^reset; and ^cyan;b^reset; are numbers between"
                            .. " ^cyan;0^reset; and ^cyan;255^reset;, inclusive."
                    end
                elseif key == "imagePath" then
                    if args[3] then
                        player.setIdentity({
                            imagePath = args[3],
                        })
                        return "Set ^cyan;imagePath^reset; to ^cyan;'" .. args[3] .. "'^reset;."
                    else
                        local newIdentity = jobject()
                        newIdentity.imagePath = nil
                        player.setIdentity(newIdentity)
                        return "Set ^cyan;imagePath^reset; to ^cyan;null^reset;."
                    end
                else
                    local keyFound = false
                    for n, idKey in ipairs(identityKeys) do
                        if key == idKey then keyFound = true end
                    end
                    if not keyFound then
                        return "Invalid key. Valid keys are:\n"
                            .. getIdentitySetCommands()
                            .. "\nNot specifying ^cyan;<string>^reset; for ^cyan;imagePath^reset; sets it to ^cyan;null^reset;."
                            .. "\nFor ^cyan;name^reset;, the new name must be quoted or escaped if it contains any spaces."
                    end
                    if args[3] then
                        if key == "gender" and args[3] ~= "male" and args[3] ~= "female" then
                            return "Nonexistent gender. Syntax is ^cyan;/set gender [male/female]^reset;."
                        end
                        local nameArg = chat.parseArguments(rawArgs)[3]
                        -- for n = 3, #args, 1 do
                        --     nameArg = nameArg .. args[n] .. (n < #args and " " or "")
                        -- end
                        player.setIdentity({
                            [key] = key == "name" and nameArg or args[3],
                        })
                        if key == "species" then
                            return "Attempted to set ^cyan;species^reset; to ^cyan;'"
                                .. args[3]
                                .. "'^reset;.\nYour species was not changed if the new species didn't exist in your assets or mods."
                        elseif key == "name" then
                            return "Set ^cyan;name^reset; to ^cyan;'" .. nameArg .. "'^reset;."
                        else
                            return "Set ^cyan;" .. key .. "^reset; to ^cyan;'" .. args[3] .. "'^reset;."
                        end
                    else
                        if key == "gender" then
                            return "Missing argument. Syntax is ^cyan;/set gender [male/female]^reset;."
                        elseif key == "name" then
                            return "Missing argument. Syntax is ^cyan;/set name [name]^reset;. The new name may contain spaces; quotes will show up in the name."
                        end
                        return "Missing argument. Syntax is ^cyan;/set " .. key .. " [string]^reset;."
                    end
                end
            end
        elseif args[1] == "get" then
            local key = args[2]
            if not key then
                local setHelpMessage = "^#f33;</identity get>^reset;\nAvailable keys:\n" .. getIdentityGetCommands()
                return setHelpMessage
            else
                local keyFound = false
                for _, idKey in ipairs(identityKeys) do
                    if key == idKey then keyFound = true end
                end
                if not keyFound then return "Invalid key. Valid keys are:\n" .. getIdentityGetCommands() end
                return "^cyan;" .. key .. "^reset;: ^orange;" .. sb.printJson(player.identity()[key]) .. "^reset;"
            end
        else
            return [===[Invalid subcommand. Valid subcommands:
Get an identity key: ^cyan;/identity get [subcommand]^reset; (see ^cyan;/identity get^reset; for valid keys)
Set an identity key: ^cyan;/identity set [subcommand]^reset; (see ^cyan;/identity set^reset; for valid keys)]===]
        end
    end
end

command("identity", handleIdentity)

-- /render --

local function absolutePath(itemConfig, path)
    if path:sub(1, 1) == "/" then
        return path
    else
        return itemConfig.directory .. path
    end
end

local function resolveDirectives(itemConfig)
    if itemConfig.parameters.directives then
        return itemConfig.parameters.directives
    else
        local colourIndex = itemConfig.parameters.colorIndex or 0
        local colourOptionsForIndex = {}
        if type(itemConfig.config.colorOptions) == "table" then
            if type(itemConfig.config.colorOptions[colourIndex]) == "table" then
                colourOptionsForIndex = itemConfig.config.colorOptions[colourIndex]
            end
        end
        local directives = "?replace;"
        local optionCount = 0
        for left, right in pairs(colourOptionsForIndex) do
            optionCount = optionCount + 1
            directives = directives .. left .. "=" .. right .. ";"
        end
        return optionCount == 0 and "" or directives
    end
end

local function resolveName(itemConfig)
    return itemConfig.parameters.shortdescription or itemConfig.config.shortdescription or itemConfig.config.itemName
end

local function resolveFacialHair()
    local identity = player.identity()
    local facialHairDirectives
    if identity.facialHairType ~= "" and identity.facialHairGroup ~= "" then
        facialHairDirectives = "/humanoid/"
            .. identity.species
            .. "/"
            .. identity.facialHairGroup
            .. "/"
            .. identity.facialHairType
            .. ".png:normal"
            .. identity.facialHairDirectives
    else
        facialHairDirectives = "/assetmissing.png" .. identity.facialHairDirectives
    end
    return facialHairDirectives
end

local function resolveFacialMask()
    local identity = player.identity()
    local facialMaskDirectives
    if identity.facialMaskType ~= "" and identity.facialMaskGroup ~= "" then
        facialMaskDirectives = "/humanoid/"
            .. identity.species
            .. "/"
            .. identity.facialMaskGroup
            .. "/"
            .. identity.facialMaskType
            .. ".png:normal"
            .. identity.facialMaskDirectives
    else
        facialMaskDirectives = "/assetmissing.png" .. identity.facialMaskDirectives
    end
    return facialMaskDirectives
end

local function resolveHair()
    local identity = player.identity()
    local hairDirectives
    if identity.hairType ~= "" and identity.hairGroup ~= "" then
        hairDirectives = "/humanoid/"
            .. identity.species
            .. "/"
            .. identity.hairGroup
            .. "/"
            .. identity.hairType
            .. ".png:normal"
            .. identity.hairDirectives
    else
        hairDirectives = "/assetmissing.png" .. identity.hairDirectives
    end
    return hairDirectives
end

local function resolveMask(itemConfig)
    local baseMask = itemConfig.config.mask or "/assetmissing.png"
    local replacementMask = itemConfig.parameters.mask or ""
    if replacementMask:sub(1, 1) ~= "?" then
        return replacementMask
    else
        return baseMask .. replacementMask
    end
end

local function resolveMaskLayering(base, mask) return base .. "?addmask=" .. mask end

local function resolveClothingAsset(itemDescriptor, gender, maskType)
    if itemDescriptor then
        local itemType = root.itemType(itemDescriptor.name)
        if
            itemType == "headarmor"
            or itemType == "chestarmor"
            or itemType == "legsarmor"
            or itemType == "backarmor"
        then
            local itemConfig = root.itemConfig(itemDescriptor)
            local directives = resolveDirectives(itemConfig)
            if itemType == "chestarmor" then
                local frameConfig = itemConfig.config[gender .. "Frames"] or {}
                local chest = absolutePath(itemConfig, frameConfig.body or "/assetmissing.png") .. directives
                local backSleeve = absolutePath(itemConfig, frameConfig.backSleeve or "/assetmissing.png") .. directives
                local frontSleeve = absolutePath(itemConfig, frameConfig.frontSleeve or "/assetmissing.png")
                    .. directives
                return {
                    [resolveName(itemConfig) .. " (chest)"] = chest,
                    [resolveName(itemConfig) .. " (back sleeve)"] = backSleeve,
                    [resolveName(itemConfig) .. " (front sleeve)"] = frontSleeve,
                }
            elseif itemType == "headarmor" then
                local frameConfig = itemConfig.config[gender .. "Frames"] or "/assetmissing.png"
                local hat = absolutePath(itemConfig, frameConfig) .. ":normal" .. directives
                local mask = absolutePath(itemConfig, resolveMask(itemConfig))
                if maskType == "hair" then
                    mask = resolveMaskLayering(resolveHair(), mask)
                elseif maskType == "facialhair" then
                    mask = resolveMaskLayering(resolveFacialHair(), mask)
                elseif maskType == "facialmask" then
                    mask = resolveMaskLayering(resolveFacialMask(), mask)
                end
                return { [resolveName(itemConfig) .. " (hat)"] = hat, [resolveName(itemConfig) .. " (mask)"] = mask }
            elseif itemType == "legsarmor" then
                local frameConfig = itemConfig.config[gender .. "Frames"] or "/assetmissing.png"
                local legs = absolutePath(itemConfig, frameConfig) .. directives
                return { [resolveName(itemConfig) .. " (legs)"] = legs }
            elseif itemType == "backarmor" then
                local frameConfig = itemConfig.config[gender .. "Frames"] or "/assetmissing.png"
                local back = absolutePath(itemConfig, frameConfig) .. directives
                return { [resolveName(itemConfig) .. " (back)"] = back }
            else
                return {}
            end
        else
            return {}
        end
    end
end

local function renderArmourItem(itemType, gender, maskType)
    local itemDescriptor
    if itemType == "hat" then
        itemDescriptor = player.equippedItem("headCosmetic") or player.equippedItem("head")
    elseif itemType == "chest" then
        itemDescriptor = player.equippedItem("chestCosmetic") or player.equippedItem("chest")
    elseif itemType == "legs" then
        itemDescriptor = player.equippedItem("legsCosmetic") or player.equippedItem("legs")
    elseif itemType == "back" then
        itemDescriptor = player.equippedItem("backCosmetic") or player.equippedItem("back")
    end
    if itemDescriptor then
        local directivesToRender = resolveClothingAsset(itemDescriptor, gender or player.gender(), maskType)
        local returnStrings = {}
        for imageName, directiveString in pairs(directivesToRender) do
            root.saveAssetPathToImage(directiveString, imageName, true)
            table.insert(returnStrings, "Rendered sprite to ^cyan;'$storage/sprites/" .. imageName .. ".png'^reset;.")
        end
        if #returnStrings == 0 then return "No sprites to render." end
        local returnString = ""
        for n, str in ipairs(returnStrings) do
            returnString = returnString .. str .. (n < #returnStrings and "\n" or "")
        end
        return returnString
    end
    return "No sprites to render."
end

local function resolveEmotes()
    local identity = player.identity()
    return "/humanoid/" .. identity.species .. "/emote.png" .. identity.emoteDirectives
end

local function resolveHead(gender)
    local identity = player.identity()
    return "/humanoid/" .. identity.species .. "/" .. gender .. "head.png" .. identity.bodyDirectives
end

local function resolveBody(gender)
    local identity = player.identity()
    return "/humanoid/" .. identity.species .. "/" .. gender .. "body.png" .. identity.bodyDirectives
end

local function resolveFrontArm()
    local identity = player.identity()
    return "/humanoid/" .. identity.species .. "/frontarm.png" .. identity.bodyDirectives
end

local function resolveBackArm()
    local identity = player.identity()
    return "/humanoid/" .. identity.species .. "/frontarm.png" .. identity.bodyDirectives
end

local function renderHumanoid(gender)
    gender = gender or player.gender()
    local playerName = player.name() == "" and "(no name)" or player.name()
    local directives = {
        [playerName .. " (facial hair)"] = resolveFacialHair(),
        [playerName .. " (facial mask)"] = resolveFacialMask(),
        [playerName .. " (hair)"] = resolveHair(),
        [playerName .. " (emotes)"] = resolveEmotes(),
        [playerName .. " (head)"] = resolveHead(gender),
        [playerName .. " (body)"] = resolveBody(gender),
        [playerName .. " (back arm)"] = resolveBackArm(),
        [playerName .. " (front arm)"] = resolveFrontArm(),
    }
    local returnStrings = {}
    for imageName, directiveString in pairs(directives) do
        root.saveAssetPathToImage(directiveString, imageName, true)
        table.insert(returnStrings, "Rendered sprite to ^cyan;'$storage/sprites/" .. imageName .. ".png'^reset;.")
    end
    if #returnStrings == 0 then return "No sprites to render." end
    local returnString = ""
    for n, str in ipairs(returnStrings) do
        returnString = returnString .. str .. (n < #returnStrings and "\n" or "")
    end
    return returnString
end

local function renderRawDirectives(frameMode, firstArg, secondArg)
    local directives = secondArg or firstArg
    local itemDescriptorOrName = secondArg and firstArg or nil
    local itemDescriptor
    if itemDescriptorOrName then
        local status, json = pcall(sb.parseJson, itemDescriptorOrName)
        if status and type(json) == "table" then
            itemDescriptor = json
        else
            itemDescriptor = { name = itemDescriptorOrName, count = 1 }
        end
    end
    local itemConfig = root.itemConfig(itemDescriptor)
    if directives:sub(1, 1) == "?" then
        directives = "/assetmissing.png" .. directives
    elseif directives:sub(1, 1) == ":" then
        return "A base asset must be specified in order to use a frame specifier."
    end
    root.saveAssetPathToImage(absolutePath(itemConfig, directives), "output", frameMode == "frames")
    return "Rendered sprite to ^cyan;'$storage/sprites/output.png'^reset;."
end

local function checkRenderArguments(args)
    if args[1] == "hat" or args[1] == "chest" or args[1] == "legs" or args[1] == "back" or args[1] == "body" then
        if args[2] == nil or args[2] == "male" or args[2] == "female" or args[2] == "default" then
            if
                args[1] == "hat"
                and args[2] ~= nil
                and (args[3] == "hair" or args[3] == "facialhair" or args[3] == "facialmask")
            then
                return true
            elseif not args[3] then
                return true
            else
                return false
            end
        else
            return false
        end
        return false
    else
        return true
    end
end

local noArgHelp = "^#f33;</render>^reset;\nThe following subcommands are available:\n\n"
local wrongArgHelp = "Invalid subcommand or arguments specified. Valid subcommands and arguments are:\n\n"
local renderHelp =
    [===[^cyan;/render hat <male/female/default> <hair/facialhair/facialmask>^reset;: Renders the worn head item to PNG files. The sprites are exported to ^orange;'$sprites/${shortdescription} (hat).png'^reset; and ^orange;'$sprites/${name} (mask).png'^reset;.
^cyan;/render chest <male/female/default>^reset;: Renders the worn chest/sleeves item to PNG files. The sprites are exported to ^orange;'$sprites/${shortdescription} (chest).png'^reset;, ^orange;'$sprites/${shortdescription} (back sleeve).png'^reset; and ^orange;'$sprites/${shortdescription} (front sleeve).png'^reset;.
^cyan;/render legs <male/female/default>^reset;: Renders the worn legs or chest/legs item to a PNG file. The sprite is exported to ^orange;'$sprites/${shortdescription} (legs).png'^reset;.
^cyan;/render legs <male/female/default>^reset;: Renders the worn back item to a PNG file. The sprite is exported to ^orange;'$sprites/${shortdescription} (back).png'^reset;.

^cyan;/render body <male/female/default>^reset;: Renders the character's humanoid sprites to PNG files. The sprites are exported to ^orange;'$sprites/${character name} (head).png'^reset;, ^orange;'$sprites/${character name} (emotes).png'^reset;, ^orange;'$sprites/${character name} (hair).png'^reset;, ^orange;'$sprites/${character name} (facial hair).png'^reset;, ^orange;'$sprites/${character name} (facial mask).png'^reset;, ^orange;'$sprites/${character name} (back arm).png'^reset;, ^orange;'$sprites/${character name} (front arm).png'^reset; and ^orange;'$sprites/${character name} (body).png'^reset;.

^cyan;/render <frames/noframes> [directives]^reset;: Renders the specified directives to a PNG file. The path to the base image asset must be absolute. The sprite is exported to ^orange;'$sprites/directives.png'^reset;.
^cyan;/render <frames/noframes> [item ID] [directives]^reset;: Renders the specified directives to a PNG file using the specified item ID as a 'reference' for a relative asset path. The sprite is exported to ^orange;'$sprites/${item's shortdescription or name}.png'^reset;.
^cyan;/render <frames/noframes> [JSON item descriptor] [directives]^reset;: Renders the specified directives to a PNG file using the specified item descriptor as a 'reference' for a relative asset path. The descriptor must be quoted as if you were passing it to ^cyan;/spawnitem^reset;. The sprite is exported to ^orange;'$sprites/${item's shortdescription or name}.png'^reset;.

^#f88;<note on special parameters>

^orange;$sprites^reset;: The ^cyan;sprites/^reset; folder inside your configured player/universe storage folder; this folder will be created by xSB-2 upon invoking any ^cyan;/render^reset; subcommand if it doesn't already exist.
^orange;<male/female/default>^reset;: By default, the base asset for the character's gender is used for rendering by all subcommands, but a gender may optionally be specified after a subcommand to override this. ^cyan;default^reset; uses the player's gender; this is only necessary if you're specifying the parameter below.
^orange;<hair/facialhair/facialmask>^reset;: If specified, uses the hair, facial hair or facial mask as the base for rendering a hat's mask.
^orange;<frames/noframes>^reset;: If ^cyan;frames^reset; is optionally specified, the passed directives are rendered by frame; this is necessary for, e.g., most generated clothing drawables except hats. ^cyan;noframes^reset; is the default and normally doesn't need to be explicitly specified, but is there if you end up needing to use a subcommand name as a reference item ID.

^orange;<scroll up to read>^reset;]===]

local function renderDirectives(rawArgs)
    local args = chat.parseArguments(rawArgs)
    if not args[1] then
        return noArgHelp .. renderHelp
    elseif not checkRenderArguments(args) then
        return wrongArgHelp .. renderHelp
    elseif args[1] == "hat" or args[1] == "chest" or args[1] == "legs" or args[1] == "back" then
        if args[2] == "default" then args[2] = nil end
        return renderArmourItem(args[1], args[2], args[3])
    elseif args[1] == "body" then
        if args[2] == "default" then args[2] = nil end
        return renderHumanoid(args[2])
    elseif args[1] == "frames" or args[1] == "noframes" then
        return renderRawDirectives(args[1], args[2], args[3])
    else
        return renderRawDirectives("noframes", args[1], args[2])
    end
end

command("render", renderDirectives)

-- /mute, /unmute and /listmutes --

function checkUuid(str)
    if #str ~= 32 then return false end
    if not str:match("^[a-fA-F0-9]+$") then return false end
    return true
end

local function uuidFromSpeaker(speakerId)
    local entityId = voice.speaker(speakerId) and voice.speaker(speakerId).entityId or nil
    return (entityId and world.entityExists(entityId)) and world.entityUniqueId(entityId) or nil
end

local function nameFromSpeaker(speakerId) return voice.speaker(speakerId) and voice.speaker(speakerId).name or nil end

local function uuidFromName(name)
    local speakers = voice.speakers(false)
    local speakerEntityId = nil
    for _, speaker in ipairs(speakers) do
        local status, found = pcall(string.find, speaker.name, name)
        if status and found then
            speakerEntityId = speaker.entityId
            break
        end
    end
    if speakerEntityId and world.entityExists(speakerEntityId) then
        return world.entityUniqueId(speakerEntityId)
    else
        return nil
    end
end

local function speakerFromUuid(uuid)
    local speakers = voice.speakers(false)
    local speakerId = nil
    for _, speaker in ipairs(speakers) do
        if speaker.entityId and world.entityExists(speaker.entityId) then
            if world.entityUniqueId(speaker.entityId) == uuid then
                speakerId = speaker.speakerId
                break
            end
        end
    end
    return speakerId
end

local function speakerFromName(name)
    local speakers = voice.speakers(false)
    local speakerId = nil
    for _, speaker in ipairs(speakers) do
        local status, found = pcall(string.find, speaker.name, name)
        if status and found then
            speakerId = speaker.speakerId
            break
        end
    end
    return speakerId
end

local function getSavedMutes() return root.getConfiguration("voiceMutes") or jobject() end

local function addMute(newMute)
    local mutes = root.getConfiguration("voiceMutes") or jobject()
    mutes[newMute.uuid] = {
        muted = newMute.muted,
        name = newMute.name,
    }
    root.setConfiguration("voiceMutes", mutes)
end

local function removeMute(muteToRemove)
    local mutes = root.getConfiguration("voiceMutes") or jobject()
    local removedMute, removedUuid = nil, nil
    if muteToRemove.uuid then
        removedMute, removedUuid = mutes[muteToRemove.uuid], muteToRemove.uuid
        mutes[muteToRemove.uuid] = nil
    else
        local uuidToRemove = nil
        for uuid, mute in pairs(mutes) do
            local status, found = pcall(string.find, mute.name, muteToRemove.name)
            if status and found then
                uuidToRemove = uuid
                break
            end
        end
        if uuidToRemove and mutes[uuidToRemove] then
            removedMute, removedUuid = mutes[uuidToRemove], uuidToRemove
            mutes[uuidToRemove] = nil
        end
    end
    root.setConfiguration("voiceMutes", mutes)
    return removedMute, removedUuid
end

local muteHelp = [===[^#f33;</mute>^reset;
Usage:
^cyan;/mute [name]^reset;: Mute a player by name. Spaces must be quoted or escaped.
^cyan;/mute name [name]^reset;: Same as above, but allows a user named 'name' to be muted.
^cyan;/mute uuid [uuid]^reset;: Mute a player by UUID. Must use the player's real UUID, not a vanity one.]===]

local invalidMuteHelp = [===[Invalid syntax. Usage:
^cyan;/mute [name]^reset;: Mute a player by name. Spaces must be quoted or escaped.
^cyan;/mute name [name]^reset;: Same as above, but allows a user named 'name' to be muted.
^cyan;/mute uuid [uuid]^reset;: Mute a player by UUID. Must use the player's real UUID, not a vanity one.]===]

local unmuteHelp = [===[^#f33;</unmute>^reset;
Usage:
^cyan;/unmute [name]^reset;: Unmute a player by name. Spaces must be quoted or escaped.
^cyan;/unmute name [name]^reset;: Same as above, but allows a user named 'name' to be unmuted.
^cyan;/unmute uuid [uuid]^reset;: Unmute a player by UUID. Must use the player's real UUID, not a vanity one.]===]

local invalidUnmuteHelp = [===[Invalid syntax. Usage:
^cyan;/unmute [name]^reset;: Unmute a player by name. Spaces must be quoted or escaped.
^cyan;/unmute name [name]^reset;: Same as above, but allows a user named 'name' to be unmuted.
^cyan;/unmute uuid [uuid]^reset;: Unmute a player by UUID. Must use the player's real UUID, not a vanity one.]===]

local function mute(rawArgs)
    local args = chat.parseArguments(rawArgs)
    local muteMode = "name"
    local muteArg = args[1]
    if args[1] == "name" or args[1] == "uuid" then
        muteMode = args[1]
        muteArg = args[2]
    end
    if not muteArg then
        if not args[1] then return muteHelp end
        if muteMode == "name" then
            return "Must specify the name of the user to be muted."
        else
            return "Must specify the UUID of the user to be muted."
        end
    end
    if muteMode == "name" then
        local speakerId, speakerUuid = speakerFromName(muteArg), uuidFromName(muteArg)
        if speakerId and speakerUuid then
            local speakerName = voice.speaker(speakerId) and voice.speaker(speakerId).name or muteArg
            addMute({ uuid = speakerUuid, muted = true, name = speakerName })
            voice.setSpeakerMuted(speakerId, true)
            sb.logInfo("[xSB] Muted speaker UUID '" .. speakerUuid .. "'.")
            local muteMessage = "Muted speaker ^orange;" .. speakerName .. "^reset;."
            player.queueStatusMessage(muteMessage)
            return muteMessage
        else
            local failedName = muteArg
            local failMessage
            if speakerId and voice.speaker(speakerId) then
                failedName = voice.speaker(speakerId).name
                voice.setSpeakerMuted(speakerId, true)
                sb.logInfo("[xSB] Failed to resolve name for speaker '" .. failedName .. "'.")
                failMessage = "Muted ^orange;" .. failedName .. "^reset;."
            else
                sb.logInfo("[xSB] Failed to resolve name or speaker ID for speaker '" .. failedName .. "'.")
                failMessage = "Speaker ^orange;" .. failedName .. "^reset; not found."
            end
            player.queueStatusMessage(failMessage)
            if not (speakerId and voice.speaker(speakerId)) then
                local additionalFailMessage = "Mute for ^orange;" .. failedName .. "^reset; was not saved."
                player.queueStatusMessage(additionalFailMessage)
                failMessage = failMessage .. "\n" .. additionalFailMessage
            end
            return failMessage
        end
    elseif muteMode == "uuid" then
        if not checkUuid(muteArg) then
            sb.logInfo("[xSB] Received invalid UUID '" .. muteArg .. "'.")
            return "Invalid UUID. Must specify a valid UUID."
        end
        local speakerId = speakerFromUuid(muteArg)
        local speakerName = voice.speaker(speakerId) and voice.speaker(speakerId).name or nil
        local muteMessage
        local additionalMuteMessage
        if speakerId then
            voice.setSpeakerMuted(speakerId, true)
            muteMessage = speakerName and ("Muted ^orange;" .. speakerName .. "^reset; by UUID.")
                or ("Muted UUID ^yellow;" .. muteArg .. "^reset;.")
            additionalMuteMessage = speakerName and "" or ("UUID: ^yellow;" .. muteArg .. "^reset;.")
            player.queueStatusMessage(muteMessage)
            muteMessage = muteMessage .. "\n" .. additionalMuteMessage
            if not speakerName then player.queueStatusMessage(additionalMuteMessage) end
        else
            sb.logInfo("[xSB] Failed to resolve speaker ID for UUID '" .. muteArg .. "'. Mute still saved.")
        end
        addMute({ uuid = muteArg, muted = true, name = speakerName })
        if not speakerId then
            muteMessage = "Saved mute for UUID ^yellow;" .. muteArg .. "^reset;."
            player.queueStatusMessage(muteMessage)
        end
        return muteMessage
    else
        return invalidMuteHelp
    end
end

local function unmute(rawArgs)
    local args = chat.parseArguments(rawArgs)
    local muteMode = "name"
    local muteArg = args[1]
    if args[1] == "name" or args[1] == "uuid" then
        muteMode = args[1]
        muteArg = args[2]
    end
    if not muteArg then
        if not args[1] then return unmuteHelp end
        if muteMode == "name" then
            return "Must specify the name of the user to be unmuted."
        else
            return "Must specify the UUID of the user to be unmuted."
        end
    end
    if muteMode == "name" then
        local removedMute, removedUuid = removeMute({ name = muteArg })
        if removedMute and removedUuid then
            local speakerToUnmute = speakerFromUuid(removedUuid)
            local unmuteMessage
            if speakerToUnmute then
                voice.setSpeakerMuted(speakerToUnmute, false)
                local muteName = voice.speaker(speakerToUnmute) and voice.speaker(speakerToUnmute).name
                    or (removedMute.name or nil)
                if muteName then
                    unmuteMessage = "Unmuted ^orange;" .. removedMute.name .. "^reset;."
                else
                    unmuteMessage = "Unmuted UUID ^yellow;" .. removedUuid .. "^reset;."
                end
            else
                if removedMute.name then
                    sb.logInfo(
                        "[xSB] No speaker with UUID '"
                            .. removedUuid
                            .. "' present (name '"
                            .. tostring(removedMute.name)
                            .. "'). Mute removed from saved list anyway."
                    )
                    unmuteMessage = "Unmuted ^orange;" .. removedMute.name .. "^reset;."
                else
                    sb.logInfo(
                        "[xSB] No speaker with UUID '"
                            .. removedUuid
                            .. "' present (no name). Mute removed from saved list anyway."
                    )
                    unmuteMessage = "Unmuted UUID ^yellow;" .. removedUuid .. "^reset;."
                end
            end
            player.queueStatusMessage(unmuteMessage)
            return unmuteMessage
        else
            local speakerId = speakerFromName(muteArg)
            if speakerId then
                local speakerName = voice.speaker(speakerId) and voice.speaker(speakerId).name or muteArg
                voice.setSpeakerMuted(speakerId, false)
                local uuid = uuidFromSpeaker(speakerId)
                if uuid then
                    sb.logInfo("[xSB] Unmuted speaker UUID '" .. uuid .. "'.")
                else
                    sb.logInfo("[xSB] Unmuted speaker ID " .. tostring(speakerId) .. ".")
                end
                local unmuteMessage = "Unmuted speaker ^orange;" .. speakerName .. "^reset;."
                player.queueStatusMessage(unmuteMessage)
                return unmuteMessage
            else
                sb.logInfo("[xSB] Could not resolve speaker from name '" .. muteArg("'."))
                return "Speaker ^orange;" .. muteArg .. "^reset; not found."
            end
        end
    elseif muteMode == "uuid" then
        local removedMute, removedUuid = removeMute({ uuid = muteArg })
        if removedMute and removedUuid then
            local speakerToUnmute = speakerFromUuid(removedUuid)
            local unmuteMessage
            local additionalUnmuteMessage
            if speakerToUnmute then
                voice.setSpeakerMuted(speakerToUnmute, false)
                local muteName = voice.speaker(speakerToUnmute) and voice.speaker(speakerToUnmute).name
                    or (removedMute.name or nil)
                if muteName then
                    unmuteMessage = "Unmuted ^orange;" .. removedMute.name .. "^reset; by UUID."
                    additionalUnmuteMessage = "UUID: ^yellow;" .. removedUuid .. "^reset;."
                else
                    unmuteMessage = "Unmuted UUID ^yellow;" .. removedUuid .. "^reset;."
                end
            else
                if removedMute.name then
                    sb.logInfo(
                        "[xSB] No speaker with UUID '"
                            .. removedUuid
                            .. "' present (name '"
                            .. tostring(removedMute.name)
                            .. "'). Mute removed from saved list anyway."
                    )
                    unmuteMessage = "Unmuted ^orange;" .. removedMute.name .. "^reset;."
                else
                    sb.logInfo(
                        "[xSB] No speaker with UUID '"
                            .. removedUuid
                            .. "' present (no name). Mute removed from saved list anyway."
                    )
                    unmuteMessage = "Unmuted UUID ^yellow;" .. removedUuid .. "^reset;."
                end
            end
            player.queueStatusMessage(unmuteMessage)
            if additionalUnmuteMessage then
                player.queueStatusMessage(additionalUnmuteMessage)
                return unmuteMessage .. "\n" .. additionalUnmuteMessage
            else
                return unmuteMessage
            end
        else
            local speakerId = speakerFromUuid(muteArg)
            if speakerId then
                local speakerName = voice.speaker(speakerId) and voice.speaker(speakerId).name or nil
                voice.setSpeakerMuted(speakerId, false)
                sb.logInfo("[xSB] Unmuted speaker UUID '" .. muteArg .. "'.")
                if speakerName then
                    local unmuteMessage = "Unmuted ^orange;" .. speakerName .. "^reset;."
                    local additionalUnmuteMessage = "UUID: ^yellow;" .. muteArg .. "^reset;."
                    player.queueStatusMessage(unmuteMessage)
                    player.queueStatusMessage(additionalUnmuteMessage)
                    return unmuteMessage .. "\n" .. additionalUnmuteMessage
                else
                    local unmuteMessage = "Unmuted UUID ^yellow;" .. muteArg .. "^reset;."
                    player.queueStatusMessage(unmuteMessage)
                    return unmuteMessage
                end
            else
                sb.logInfo("[xSB] Could not resolve speaker from UUID '" .. muteArg("'."))
                return "Speaker with UUID ^yellow;" .. muteArg .. "^reset; not found."
            end
        end
    else
        return invalidUnmuteHelp
    end
end

function listMutes(_)
    local mutes = getSavedMutes()
    local listing = ""
    local numberOfMutes = 0
    for uuid, mute in pairs(mutes) do
        numberOfMutes = numberOfMutes + 1
        listing = listing .. (mute.name or ("^yellow;[" .. uuid .. "]^reset;")) .. ", "
    end
    listing = listing:sub(1, -3)
    if numberOfMutes == 0 then
        return "No muted players"
    else
        local listingPrefix = "^#f33;"
            .. numberOfMutes
            .. (numberOfMutes == 1 and " muted player" or " muted players")
            .. ":^reset;\n"
        listing = listingPrefix .. listing
        return listing
    end
end

command("mute", mute)
command("unmute", unmute)
command("listmutes", listMutes)

function module:update(dt)
    if self.muteCheckInterval ~= 0 then
        self.muteCheckTimer = self.muteCheckTimer + dt
        if self.muteCheckTimer >= self.muteCheckInterval then
            self.muteCheckTimer = 0
            local mutes = getSavedMutes()
            for uuid, _ in pairs(mutes) do
                local speakerId = speakerFromUuid(uuid)
                if speakerId and not self.oldMutes[uuid] then voice.setSpeakerMuted(speakerId, true) end
            end
            for uuid, _ in pairs(self.oldMutes) do
                local speakerId = speakerFromUuid(uuid)
                if speakerId and not mutes[uuid] then voice.setSpeakerMuted(speakerId, false) end
            end
            self.oldMutes = mutes
        end
    end
end

-- /voice --

local voiceHelp = [===[^#f33;</voice>^reset;
Available subcommands:
^cyan;/voice showall [on/off]^reset;: Toggle whether to show all potential speakers in the voice UI, not just people actually speaking.
^cyan;/voice muteupdate [seconds/off]^reset;: Set the interval in seconds at which saved mutes should be updated. Set this to ^cyan;0^reset; or ^cyan;off^reset; to turn saved mutes off.]===]

local invalidVoiceHelp = [===[Invalid syntax. Valid subcommands:
^cyan;/voice showall [on/off]^reset;: Toggle whether to show all potential speakers in the voice UI, not just people actually speaking.
^cyan;/voice muteupdate [seconds/off]^reset;: Set the interval in seconds at which saved mutes should be updated. Set this to ^cyan;0^reset; or ^cyan;off^reset; to turn saved mutes off.]===]

local function voiceSettings(rawArgs)
    local args = chat.parseArguments(rawArgs)
    if not args[1] then
        return voiceHelp
    elseif args[1] == "showall" then
        if args[2] == "on" then
            root.setConfiguration("voiceShowAllSpeakers", true)
            return "Now showing all speakers."
        elseif args[2] == "off" then
            root.setConfiguration("voiceShowAllSpeakers", false)
            return "Now showing only active speakers."
        else
            return invalidVoiceHelp
        end
    elseif args[1] == "muteupdate" then
        if tonumber(args[2]) then
            local newInterval = tonumber(args[2])
            if newInterval > 0 then
                root.setConfiguration("voiceMuteCheckInterval", 1.0)
                return "Set the mute update interval to ^cyan;"
                    .. tostring(newInterval)
                    .. "^reset; second(s). Warp anywhere or relog to use the new interval."
            elseif newInterval == 0 then
                root.setConfiguration("voiceMuteCheckInterval", 0.0)
                return "Turned off saved mutes. Warp anywhere or relog to fully disable saved mutes."
            else
                return "Sorry, time travel doesn't exist, bud. The new mute update interval must be non-negative."
            end
        elseif args[2] == "off" then
            root.setConfiguration("voiceMuteCheckInterval", 0.0)
            return "Turned off saved mutes. Warp anywhere or relog to fully disable saved mutes."
        else
            return invalidVoiceHelp
        end
    end
end

command("voice", voiceSettings)

-- /description --

local function handleDescription(rawArgs)
    local args = chat.parseArguments(rawArgs)
    if not args[1] then
        local description = player.description()
        if description == "" then
            sb.logInfo("[xSB] Player description is empty.")
        else
            sb.logInfo("[xSB] Player description: %s", description)
        end
        description = description == "" and "^gray;<no description>^reset;" or description
        return "^#888,set;[" .. player.name() .. "^reset,#888,set;]^#aaa,set; " .. description
    else
        player.setDescription(args[1])
        return "Set this character's description to: '" .. args[1] .. "'"
    end
end

command("description", handleDescription)

-- /gamemode --

local invalidGameModeHelp = [===[Invalid syntax. Usage:
^cyan;/gamemode^reset;: Returns the player's current game mode.
^cyan;/gamemode [casual/survival/hardcore]^reset;: Sets the player's game mode to the one specified. The player must be an admin to do this.]===]

local function handleGameMode(rawArgs)
    local args = chat.parseArguments(rawArgs)
    if not args[1] then
        return "You are in ^orange;" .. player.mode() .. "^reset; mode"
    elseif args[1] == "casual" or args[1] == "survival" or args[1] == "hardcore" then
        if player.isAdmin() then
            player.setMode(args[1])
            return "Set the player's game mode to ^orange;" .. args[1] .. "^reset;"
        else
            return "Insufficient privileges to set the player's game mode"
        end
    else
        return invalidGameModeHelp
    end
end

command("gamemode", handleGameMode)