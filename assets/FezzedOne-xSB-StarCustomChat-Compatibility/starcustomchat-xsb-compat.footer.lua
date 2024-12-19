
-- < END OF BASE SCRIPT > --

local base_init, base_update, base_uninit = init, update, uninit

function init(...)
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