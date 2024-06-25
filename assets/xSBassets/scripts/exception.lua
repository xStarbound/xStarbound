pluto_class exception
    __name = "pluto:exception"

    function __construct(public what)
        local caller
        local i = 2
        while true do
            caller = debug.getinfo(i)
            if caller == nil then
                error("exception instances must be created with 'pluto_new'", 0)
            end
            ++i
            if caller.name == "Pluto_operator_new" then
                caller = debug.getinfo(i)
                break
            end
        end
        self.where = $"{caller.short_src}:{caller.currentline}"
        error(self, 0)
    end

    function __tostring()
        return $"{self.where}: {tostring(self.what)}"
    end
end

function instanceof(a, b)
  return a instanceof b
end