local module = {}
modules.messages = module

function module:init()
    message.setHandler("keepAlive", function(_, _, time)
        return world.setExpiryTime(tonumber(time) or 0)
    end)

	message.setHandler("metadata", function(_, isLocal)
        if root.getConfiguration("allowWorldMetadataChanges") then
            return world.metadata()
        end
    end)

	message.setHandler("setMetadata", function(_, isLocal, newMetadata)
        if root.getConfiguration("allowWorldMetadataChanges") then
            return world.setMetadata(newMetadata)
        end
    end)
end

function module:update()

end