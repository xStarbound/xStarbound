local module = {}
modules.messages = module

function module:init()
    message.setHandler("keepAlive", function(_, _, time)
        return world.setExpiryTime(tonumber(time) or 0)
    end)

	message.setHandler("metadata", function(_, _)
        return world.metadata()
    end)

	message.setHandler("setMetadata", function(_, _, newMetadata)
        return world.setMetadata(newMetadata)
    end)
end

function module:update()

end