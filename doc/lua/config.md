# `config`

The `config` table is available in the following script contexts:

- active item scripts
- augment scripts
- container interface scripts
- fireable item scripts
- NPC scripts
- object scripts
- monster scripts
- pane scripts
- player companion scripts
- player deployment scripts
- projectile scripts
- quest scripts
- stagehand scripts
- status effect scripts
- tech scripts
- vehicle scripts

Each entity type (item type, object type, monster type, NPC variant, status effect kind, etc.) in the loaded assets within a given context has its own config, with the following exceptions and caveats:

- Augment scripts have a config for each augment type in the loaded assets.
- Companion scripts have a single config for all companions a player has.
- Statistics scripts have a config for each achievement or other statistics event in the loaded assets.
- Tech scripts have a config for each tech type in the loaded assets.
- Container interface and pane scripts have a config for each configured pane or container interface type, respectively, in the loaded assets.
- Item scripts have a config for each and every actual item (or item stack); items of the same type in different stacks have their own per-stack configs, saved to the item stacks themselves. If a config parameter in an item's instance parameters is `null` or not present, the value, if any, in the item's asset config is returned.
- Item scripts have a config for each and every actual placed object; objects of the same type have their own per-object configs, saved to the objects themselves. If a config parameter in an object's instance parameters is `null` or not present, the value, if any, in the object's asset config is returned.

---

#### `Json` config.getParameter(`String` parameter, `Json` default)

Returns the value for the specified asset config parameter, which may be specified as a JSON path (see `root.md`). If the specified config key does not exist, returns the default, or `nil` if no default is specified. Additionally, in item and object scripts, returns the default or `nil` if the specified asset/instance config key exists and has a value of `null`.