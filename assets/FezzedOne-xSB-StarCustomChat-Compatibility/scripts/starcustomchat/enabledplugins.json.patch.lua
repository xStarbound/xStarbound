function patch(original) -- Is a Lua script to avoid compatibility issues with stock clients.
    if xsb then
        local newPlugins = jarray{}
        for _, pluginName in ipairs(original) do
            if pluginName ~= "voicechat" then
                table.insert(newPlugins, pluginName)
            end
        end
        return newPlugins
    else
        return original
    end
end