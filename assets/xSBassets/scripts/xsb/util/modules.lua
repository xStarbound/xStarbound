-- Small helper to organize code for the same context into different Lua scripts without having to "hook" previously defined.
-- FezzedOne: Passed modules' own tables into the modules themselves, allowing the use of colon syntax and `self` variables.

modules = setmetatable({}, {__call = function(this, path, names)
	for i, name in pairs(names) do
		require(path .. name .. ".lua")
	end
end})

local modules, type = modules, type
local function call(func, ...)
	if type(func) == "function" then
		return func(...)
	end
end

function init(...)
	script.setUpdateDelta(1)
	for i, module in pairs(modules) do
		call(module.init, module, ...)
	end
end

function update(...)
	for i, module in pairs(modules) do
		call(module.update, module, ...)
	end
end

function uninit(...)
	for i, module in pairs(modules) do
		call(module.uninit, module, ...)
	end
end