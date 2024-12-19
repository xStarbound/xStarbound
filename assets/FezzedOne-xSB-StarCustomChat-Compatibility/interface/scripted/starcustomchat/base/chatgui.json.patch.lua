function patch(original)
    if xsb then -- Fixed the chat pane so that Esc opens the escape menu as it should.
        local patched = original
        patched.paneLayer = "hud"
        return patched
    else
        return original
    end
end