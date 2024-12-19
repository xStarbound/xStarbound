
-- < END OF BASE SCRIPT > --

-- Fix for Bigger Chat not pasting its text into the regular chatbox. This is a bug in the original SCC code!

function uninit()
    player.setProperty("biggerChatOpen", nil)
    world.sendEntityMessage(player.id(), "bigger_chat_close_picker")
    pane.dismiss()
end