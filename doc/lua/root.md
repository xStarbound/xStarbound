# `root`

The `root` table contains functions that reference the game's currently loaded assets and don't relate to any more specific context such as a particular world or universe. Available in all script contexts except patch and preprocessor scripts, which have access to the `assets` table instead (see `assets.md` and `$docs/patch.md`).

## Paths

Below is a detailed explanation of how paths work in xSB-2 (and stock Starbound).

### File paths

*File paths* are paths within the OS's filesystem. These use your OS's conventions for filesystem paths. Note that relative file paths are relative to the directory containing the running xSB-2 (or stock) executable. Some notes for Linux and Windows:

  - On Linux, file and directory names are case-sensitive by default, only forward slashes (`/`) are parsed as path separators, the special names `.` and `..` are disallowed (but may still be used with their usual special meaning in paths), and on most Linux filesystems, any character other than a null byte (`\0`) or forward slash is allowed in file and directory names.
  - On Windows, file and directory names are case-insensitive by default, both backward (`\`) and forward (`/`) slashes are parsed as path separators, drive letters may need to be specified, several characters are not allowed in file and directory names, and multiple other caveats apply. See [this link](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file) for details.

### Asset paths

*Asset paths* are paths within Starbound's internal asset filesystem. These have the following characteristics:

  - The path separator is a forward slash (`/`), and file and directory names are case-insensitive, even on Linux.
  - `.` and `..` have no special meaning in asset paths. (Because of this, they are *technically* allowed as regular asset file and folder names; see the note below.)
  - Any character other than a null byte (`\0`), question mark (`?`), colon (`:`) or forward slash is allowed in an asset file or directory name, although it is strongly recommended to stick to alphanumeric characters to avoid OS compatibility issues.
  - The colon (`:`) is a special character that marks the beginning of a subpath specifier. For JSON assets, this is a JSON path (see below), while for image assets, this is a frame specifier. For other assets, whatever follows `:` is ignored. Some callbacks noted below (and in `assets.md`) disallow subpaths (and therefore `:`) in paths entirely.
  - The question mark (`?`) is a special character that marks the beginning of image preprocessing directives and also splits individual directive operations. See `$docs/image.md` for more details on image preprocessing. Preprocessing directives must follow any subpath specifier if present. For non-image assets, whatever follows `?` is ignored. Some callbacks noted below (and in `assets.md`) disallow directives (and therefore `?`) in paths entirely.
  - Asset paths in animation configs additionally treat angle brackets (`<` and `>`) as special characters that wrap animation tag specifiers. See below for details. Note that despite this, the characters `<` and `>` are still permitted (but not recommended!) in asset file and directory names.
  - JSON asset configs, but *not* Lua callbacks, may specify a relative path by omitting the first slash; the path is relative to the location of the JSON config in which the relative path is specified.

> **Technical note:** Some technical caveats to what's explained above:
>
> - Actually, `?` and `:` characters are *technically* allowed in asset paths passed to `assets` callbacks. However, since directories and files with these characters in their names can't be accessed outside of asset preprocessor scripts anyway, there's little reason to bother with this.
> - `assets.add` is the only way to create an asset directory or file named `.` or `..`, since most (or all?) OSes do not allow files or directories with those names to be created.

### JSON paths

JSON paths are optionally specified after a colon in an asset path, and some callbacks accept JSON paths directly. The path string syntax is as follows:

- `k`: A JSON object «root» key, where `k` is substituted for the literal key string, without quotes; e.g., `parameters`.
- `.k`: A JSON object «subdirectory» key; e.g., `.animationConfig`. Used to access the value associated with a given key inside an object associated with the given «root» or «subdirectory» key. `k` is substituted as for «root» keys above. A path string may *not* begin with a subdirectory key.
- `[x]`: A JSON array index specifier, where `x` is either an unsigned integer or `-` (refers to the last index in the array). Used to access the value at the specified array index. Indices begin at `0`, not `1`. Path strings *may* begin with an array index, assuming the «root» is an array (as in many JSON patch files).

Keys and index specifiers may be freely concatenated as in Lua code; e.g., `"actionOnReap[2].action"` accesses the value of the key `"action"` of an object at the third index of the array `"actionOnReap"` at the «root» of the config.

**Note:** The following caveats apply:

- Since `.` and `[` are always parsed as operators, object keys containing any `.` or `[` characters cannot be specified in a JSON path and must instead be directly accessed in Lua.
- In asset paths, `?` always terminates a subpath, and as such, object keys containing `?` characters cannot be specified in an asset path containing a JSON subpath and have to be directly accessed in Lua instead.

Because of this and other considerations, JSON keys containing non-alphanumeric characters should be avoided in Starbound JSON.

### Animation tags

Asset paths in animation configs treat angle brackets (`<` and `>`) as special characters that wrap tag specifiers; these wrapped specifiers are called *animation tags*. The engine replaces animation tags before doing any other path processing in the following circumstances:

  - *`<effectDirectives>`:* In player effects animators, replaced with the value of `"effectiveDirectives"` specified in the appropriate `.species` config file.
  - *`<color>`:* In all object animation configs, replaced with the value of the object's saved `"color"` parameter.
  - *`<state>` and `<key>`:* In container object animation configs, replaced with the container's current state (`crafting` or `idle`) and the current animation frame number (e.g., `1`, `2`, etc.), respectively.
  - *`<stage>` and `<alt>`:* In farmable object animation configs, replaced with the farmable's current primary and alternate stage names.
  - *Any other tag:* `animator.setPartTag` and `animator.setGlobalTag` (and the equivalents in `playerAnimator`) replace any tag matching the given specifier with the given replacement string, either only for a specific animation part or for the entire animation config.

Tags that aren't replaced are left untouched, potentially allowing angle brackets to remain in the path after tag processing.

## Path types

The following path type names are used throughout this documentation to refer to path strings with differing restrictions:

- **`AssetPath<Subpath, Directives, Tags>`:** A standard asset path where a subpath and/or directives are permitted and `< >` tags may be replaced. Only used in JSON animation configs. (`?` and `:` allowed.)
- **`AssetPath<Subpath, Directives>`:** A standard asset path where a subpath and/or directives are permitted, as detailed above. (`?` and `:` allowed.)
- **`AssetPath<Subpath>`:** An asset path that allows a subpath but *not* directives. (`:`, but *not* `:`, allowed.)
- **`AssetPath<>`:** An asset path that disallows *both* subpaths and directives. (Neither `?` nor `:` allowed.)
- **`RawAssetPath`:** A raw asset path where no special meaning is assigned to `?` or `:`.
- **`FilePath`:** An OS file path as detailed above.

---

#### `List<AssetPath<>>` root.assetsByExtension(`String` extension)

Returns a list of all assets that have a given file extension. Extension matches are case-insensitive; any initial period in the specified extension is optional.

---

#### `Maybe<FilePath>` root.assetSource(`AssetPath<>` assetPath)

Returns the file path to the asset source containing the loaded version of the specified asset path, or `nil` if no asset exists at that path.

---

#### `Json` root.assetJson(`AssetPath<Subpath>` assetPath)

Returns the contents of the specified JSON asset file.

---

#### `Json` root.makeCurrentVersionedJson(`String` versioningIdentifier, `Json` content)

Returns a versioned JSON representation of the given JSON content with the given identifier and the most recent version as specified in `versioning.config`.

---

#### `Json` root.loadVersionedJson(`Json` versionedContent, `String` versioningIdentifier)

Returns the given JSON content and identifier after applying appropriate versioning scripts to bring it up to the most recent version as specified in `versioning.config`.

---

#### `double` root.evalFunction(`String` functionName, `double` input)

Returns the evaluation of the specified univariate function (as defined in a `.functions` file) for the given input value.

---

#### `double` root.evalFunction2(`String` functionName, `double` input1, `double` input2)

Returns the evaluation of the specified bivariate function (as defined in a `.2functions` file) for the given input values.

---

#### `Vec2U` root.imageSize(`AssetPath<Subpath, Directives>` imagePath)

Returns the pixel dimensions of the specified image asset.

---

#### `List<Vec2I>` root.imageSpaces(`AssetPath<Subpath, Directives>` imagePath, `Vec2F` worldPosition, `float` spaceScan, `bool` flip)

Returns a list of the world tile spaces the image would occupy if placed at the given position using the specified `spaceScan` value (the portion of a space that must be non-transparent for that space to count as filled).

---

#### `RectU` root.nonEmptyRegion(`AssetPath<Subpath, Directives>` imagePath)

Returns the rectangle containing the portion of the specified asset image that is non-transparent.

---

#### `Json` root.npcConfig(`String` npcType)

Returns a representation of the generated JSON configuration for an NPC of the given type.

---

#### `Json` root.npcVariant(`String` species, `String` npcType, `float` level, [`unsigned` seed], [`Json` parameters])

Generates an NPC with the specified species, type, level, seed and parameters, and returns its configuration.

---

#### `float` root.projectileGravityMultiplier(`String` projectileName)

Returns the gravity multiplier of the given projectile's movement controller configuration as configured in `physics.config`.

---

#### `Json` root.projectileConfig(`String` projectileName)

Returns a representation of the JSON configuration for the given projectile.

---

#### `Json` root.itemDescriptorsMatch(`ItemDescriptor` descriptor1, `ItemDescriptor` descriptor2, [`bool` exactMatch])

Returns `true` if the given item descriptors match. If exactMatch is `true` then both names and parameters will be compared, otherwise only names.

---

#### `JsonArray` root.recipesForItem(`String` itemName)

Returns a list of JSON configurations of all recipes which output the given item.

---

#### `String` root.itemType(`String` itemName)

Returns the item type name for the specified item.

---

#### `JsonArray` root.itemTags(`String` itemName)

Returns a list of the tags applied to the specified item.

---

#### `bool` root.itemHasTag(`String` itemName, `String` tagName)

Returns true if the given item's tags include the specified tag and false otherwise.

---

#### `Json` root.itemConfig(`ItemDescriptor` descriptor, [`float` level], [`unsigned` seed])

Generates an item from the specified descriptor, level and seed and returns a JSON object containing the `directory`, `config` and `parameters` for that item.

---

#### `ItemDescriptor` root.createItem(`ItemDescriptor` descriptor, [`float` level], [`unsigned` seed])

Generates an item from the specified descriptor, level and seed and returns a new item descriptor for the resulting item.

---

#### `Json` root.tenantConfig(`String` tenantName)

Returns the JSON configuration for the given tenant.

---

#### `JsonArray` root.getMatchingTenants(`map<String, unsigned>` colonyTags)

Returns an array of JSON configurations of tenants matching the given map of colony tags and corresponding object counts.

---

#### `JsonArray` root.liquidStatusEffects(`LiquidId` liquid)

Returns an array of status effects applied by the given liquid.

---

#### `String` root.generateName(`String` assetPath, [`unsigned` seed])

Returns a randomly generated name using the specified name gen config and seed.

---

#### `Json` root.questConfig(`String` questTemplateId)

Returns the JSON configuration of the specified quest template.

---

#### `JsonArray` root.npcPortrait(`String` portraitMode, `String` species, `String` npcType, `float` level, [`unsigned` seed], [`Json` parameters])

Generates an NPC with the specified type, level, seed and parameters and returns a portrait in the given portraitMode as a list of drawables.

---

#### `JsonArray` root.monsterPortrait(`String` typeName, [`Json` parameters])

Generates a monster of the given type with the given parameters and returns its portrait as a list of drawables.

---

#### `bool` root.isTreasurePool(`String` poolName)

Returns true if the given treasure pool exists and false otherwise. Can be used to guard against errors attempting to generate invalid treasure.

---

#### `JsonArray` root.createTreasure(`String` poolName, `float` level, [`unsigned` seed])

Generates an instance of the specified treasure pool, level and seed and returns the contents as a list of item descriptors.

---

#### `String` root.materialMiningSound(`String` materialName, [`String` modName])

Returns the path of the mining sound asset for the given material and mod combination, or `nil` if no mining sound is set.

---

#### `String` root.materialFootstepSound(`String` materialName, [`String` modName])

Returns the path of the footstep sound asset for the given material and mod combination, or `nil` if no footstep sound is set.

---

#### `float` root.materialHealth(`String` materialName)

Returns the configured health value for the specified material.

---

#### `Json` root.materialConfig(`String` materialName)

Returns a JSON object containing the `path` and base `config` for the specified material if it is a real material, or `nil` if it is a metamaterial or invalid.

---

#### `Json` root.modConfig(`String` modName)

Returns a JSON object containing the `path` and base `config` for the specified mod if it is a real mod, or `nil` if it is a metamod or invalid.

---

#### `Json` root.liquidConfig(`LiquidId` liquidId)

#### `Json` root.liquidConfig(`String` liquidName)

Returns a JSON object containing the `path` and base `config` for the specified liquid name or id if it is a real liquid, or `nil` if the liquid is empty or invalid. A `LiquidId` is an integer liquid ID as specified in the liquid's config file.

---

#### `String` root.liquidName(`LiquidId` liquidId)

Returns the string name of the liquid with the given ID.

---

#### `LiquidId` root.liquidId(`String` liquidName)

Returns the numeric ID of the liquid with the given name.

---

#### `Json` root.monsterSkillParameter(`String` skillName, `String` parameterName)

Returns the value of the specified parameter for the specified monster skill.

---

#### `Json` root.monsterParameters(`String` monsterType, [uint64_t seed])

Returns the parameters for a monster type.

---

#### `ActorMovementParameters` root.monsterMovementSettings(`String` monsterType, [uint64_t seed])

Returns the configured base movement parameters for the specified monster type.

---

#### `Json` root.createBiome(`String` biomeName, `unsigned` seed, `float` verticalMidPoint, `float` threatLevel)

Generates a biome with the specified name, seed, vertical midpoint and threat level, and returns a JSON object containing the configuration for the generated biome.

---

#### `String` root.hasTech(`String` techName)

Returns `true` if a tech with the specified name exists and `false` otherwise.

---

#### `String` root.techType(`String` techName)

Returns the type (tech slot) of the specified tech.

---

#### `Json` root.techConfig(`String` techName)

Returns the JSON configuration for the specified tech.

---

#### `String` root.treeStemDirectory(`String` stemName)

Returns the path within assets from which the specified tree stem type was loaded.

---

#### `String` root.treeFoliageDirectory(`String` foliageName)

Returns the path within assets from which the specified tree foliage type was loaded.

---

#### `Collection` root.collection(`String` collectionName)

Returns the metadata for the specified collection.

---

#### `List<Collectable>` root.collectables(`String` collectionName)

Returns a list of collectables for the specified collection.

---

#### `String` root.elementalResistance(`String` elementalType)

Returns the name of the stat used to calculate elemental resistance for the specified elemental type.

---

#### `Json` root.dungeonMetadata(`String` dungeonName)

Returns the metadata for the specified dungeon definition.

---

#### `BehaviorState` root.behavior(`LuaTable` context, `Json` config, `JsonObject` parameters, `Maybe<Blackboard>` blackboard)

Loads a configured behaviour and returns the behaviour state as a userdata `BehaviorState` object. Explanation of the parameters:

- `context` is the current lua context called from; in most cases, this should be `_ENV` or perhaps `_G`.
- `config` can be either the `String` name of a behaviour tree, or an entire `JsonObject` behaviour tree configuration to be built.
- `parameters` may contain overrides for parameters for the behaviour tree.
- `blackboard` is an optionally specified existing `Blackboard` object to use for running the behaviour.

---

## `BehaviorState` objects

`BehaviorState` objects have the following methods. All methods take the object itself as the first argument, so use the `:` syntax.

Example:

```lua
function init()
  self.behavior = root.behavior(_ENV, "monster", {})
end

function update(dt)
  self.behavior:run(dt)
end
```

---

#### `Blackboard` `[BehaviorState]`:blackboard()

Returns a `Blackboard` object (see below) for the behaviour. Will throw an exception if the behaviour has expired.

---

#### `void` `[BehaviorState]`:run(`DeltaTime` dt)

Runs the behaviour for a single tick. Will throw an exception if the behaviour has expired.

---

#### `void` `[BehaviorState]`:clear()

Resets the internal state of the behaviour. Will throw an exception if the behaviour has expired.

---

## `Blackboard` objects

`Blackboard` objects have the following methods. All methods take the object itself as the first argument, so use the `:` syntax.

### `NodeValue` and `NodeType`

`NodeType` is a `String` type name corresponding to a given Lua value type (`NodeValue`). The type names and their corresponding Lua value types are as specified below.

- `"json"`: Any one of a JSON array (created with `jarray`), JSON object (created with `jobject`), string, number or boolean.
- `"entity"`: An integer entity ID.
- `"position"`: A `Vec2` representing a world position. E.g., `jarray{1.0, 2.0}`.
- `"vec2"`: Any other `Vec2`. E.g., `jarray{1.0, 2.0}`.
- `"number"`: A float or integer number.
- `"bool"`: A boolean.
- `"list"`: A JSON array. E.g., `jarray{"foo", "bar", "baz"}`.
- `"table"`: A JSON object. E.g., `jobject{foo = 1, bar = jarray{}, baz = false}`.
- `"string"` A string.

Note that a `NodeValue` of any type may be `nil`.

---

#### `NodeValue` `[Blackboard]`:get(`NodeType` type, `String` key)

Gets the value stored in the node whose key and type is specified. Throws an exception if the specified type does not match the key's actual type.

---

#### `NodeValue` `[Blackboard]`:set(`NodeType` type, `String` key, `NodeValue` value)

Stores the specified value of the specified type in the node whose key is specified, creating the node if it doesn't already exist; if the value is `nil`, removes the node. Note that keys of different node types may share the same name. Normally throws an exception if the specified type does not match the key's actual type.

**Caveat:** If a key to a `"vec2"` (but *not* `"position"`) node is specified, but the specified type is a `"number"`, the given number value is assigned to both values in that `"vec2"` node *in addition to* being assigned to a `"number"` node with the same name. For instance, a value of `2.5` becomes `jarray{2.5, 2.5}` in a `"vec2"` node.