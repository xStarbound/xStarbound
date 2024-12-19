function patch(original)
    -- Replaced StarExtensions command with xStarbound commands.
    local patched = jobject({
        general = original.general,
        admin = jarray({ table.unpack(original.admin), "/listworld", "/serverexec", "/serverrun" }),
        debug = jarray({ table.unpack(original.debug), "/exec", "/run" }),
        starcustomchat = original.starcustomchat,
        xclient = jarray({
            "/add",
            "/adduuid",
            "/description",
            "/gamemode",
            "/identity",
            "/mute",
            "/render",
            "/remove",
            "/removeuuid",
            "/swap",
            "/swapuuid",
            "/unmute",
            "/voice",
            "/xclient",
        }),
        xserver = jarray({ "/world", "/xserver" }),
    })
    return patched
end
