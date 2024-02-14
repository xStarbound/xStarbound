local module = {}
modules.commands = module

local commands = {}
local function command(name, func)
  commands[name] = func
end

function module.init()
  for name, func in pairs(commands) do
    message.setHandler({name = "/" .. name, localOnly = true}, func)
  end
end


command("run", function(src)
  if load then
    local success, result = pcall(load, src, "/run", "t", _ENV)
    if not success then
      return "^#f00;Compiler error: " .. result
    else
      local success, result = pcall(result)
      if not success then
        return "^#f00;Error: " .. result
      else
        local success, printed = pcall(sb.printJson, result)
        if not success then
          success, printed = pcall(sb.print, result)
        end
        if not success then
          return "^#f00;Could not print return value: " .. printed
        else
          return printed
        end
      end
    end
  else
    return "\"safeScripts\" must be disabled in xclient.config in order to use this command."
  end
end)