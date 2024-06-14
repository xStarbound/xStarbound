# config

The `config` lua bindings relate to anything that has a configuration and needs to access configuration parameters.

---

#### `Json` config.getParameter(`String` parameter, `Json` default)

Returns the value for the specified asset config parameter, which may be specified as a JSON path (see `root.md`). If the specified config key does not exist, returns the default, or `nil` if no default is specified. Additionally, in item and object scripts, returns the default or `nil` if the specified asset/instance config key exists and has a value of `null`.
