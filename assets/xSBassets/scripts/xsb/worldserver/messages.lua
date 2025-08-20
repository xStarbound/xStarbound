local module = {}
modules.messages = module

function module:init()
    message.setHandler("keepAlive", function(_, _, time) return world.setExpiryTime(tonumber(time) or 0) end)

    message.setHandler("builder::metadata", function(_, isLocal)
        if isLocal or root.getConfiguration("allowWorldMetadataChanges") then return world.metadata() end
    end)

    message.setHandler("builder::setMetadata", function(_, isLocal, newMetadata)
        if isLocal or root.getConfiguration("allowWorldMetadataChanges") then return world.setMetadata(newMetadata) end
    end)
end

function module:update() end

