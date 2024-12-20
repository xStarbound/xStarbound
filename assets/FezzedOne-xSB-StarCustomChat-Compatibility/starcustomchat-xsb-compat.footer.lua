
-- < END OF BASE SCRIPT > --

local base_init, base_update, base_uninit = init, update, uninit

function init(...)
    local base_root_imageSize = root.imageSize
    -- Needs to be baby-proofed because SCC doesn't sanitise input or handle exceptions from this callback at all. Ugh.
    root.imageSize = function(imagePath)
        local status, result = pcall(base_root_imageSize, imagePath)
        if status then
            return result
        else
            sb.logWarn("[StarCustomChat] Error getting image size: %s", result)
            return {0, 0}
        end
    end

    shared = getmetatable('').shared
    handleSmuggledHandlers()
    if type(base_init) == "function" then
        return base_init(...)
    end
    return nil  
end

function update(...)
    shared = getmetatable('').shared
    handleSmuggledHandlers()
    if type(base_update) == "function" then
        return base_update(...)
    end
    return nil
end

function uninit(...)
    shared = getmetatable('').shared
    handleSmuggledHandlers()
    if type(base_uninit) == "function" then
        return base_uninit(...)
    end
    return nil    
end