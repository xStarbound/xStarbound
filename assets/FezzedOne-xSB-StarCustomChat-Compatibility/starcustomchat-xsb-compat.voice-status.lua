
-- < END OF BASE SCRIPT > --

local soundCompatVarName = "SCCCompat::SmuggledSoundMessage"

function update(dt)
    if xsb then
        local soundData = world.getGlobal(soundCompatVarName)
        if soundData then
            sccTalkingSound(soundData)
            world.setGlobal(soundCompatVarName, nil)
        end
    end
end