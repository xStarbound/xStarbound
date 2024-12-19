function patch(original)
    if xsb then -- Fixed the chat reveal button so that Esc no longer permanently hides it (and also opens the escape menu as it should).
        local patched = original
        patched.paneLayer = "hud"
        return patched
    else
        return original
    end
end