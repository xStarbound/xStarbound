# `root`

The `root` table contains functions that reference the game's currently loaded assets and don't relate to any more specific context such as a particular world or universe. Available in all script contexts except patch and preprocessor scripts, which have access to the `assets` table instead (see `assets.md` and `$docs/patch.md`).

## Paths

Below is a detailed explanation of how paths work on xStarbound (and stock Starbound).

### File paths

*File paths* are paths within the OS's filesystem. These use your OS's conventions for filesystem paths. Note that relative file paths are relative to the directory containing the running xStarbound (or stock) executable. Some notes for Linux and Windows:

  - On Linux, file and directory names are case-sensitive by default, only forward slashes (`/`) are parsed as path separators, the special names `.` and `..` are disallowed (but may still be used with their usual special meaning in paths), and on most Linux filesystems, any character other than a null byte (`\0`) or forward slash is allowed in file and directory names.
  - On Windows, file and directory names are case-insensitive by default, both backward (`\`) and forward (`/`) slashes are parsed as path separators, drive letters may need to be specified, several characters are not allowed in file and directory names, and multiple other caveats apply. See [this link](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file) for details.

### Asset paths

*Asset paths* are paths within Starbound's internal asset filesystem. These have the following characteristics:

  - The path separator is a forward slash (`/`), and file and directory names are case-insensitive, even on Linux.
  - `.` and `..` have no special meaning in asset paths. (Because of this, they are *technically* allowed as regular asset file and folder names; see the note below.)
  - Any character other than a null byte (`\0`), question mark (`?`), colon (`:`) or forward slash is allowed in an asset file or directory name, although it is strongly recommended to stick to alphanumeric characters to avoid OS compatibility issues.
  - The colon (`:`) is a special character that marks the beginning of a subpath or frame specifier. For JSON assets, this is a JSON subpath (see below), while for image assets, this is a frame specifier (see below). For other assets, whatever follows `:` is ignored. Some callbacks noted below (and in `assets.md`) disallow subpaths (and therefore `:`) in paths entirely.
  - The question mark (`?`) is a special character that marks the beginning of image preprocessing directives and also splits individual directive operations. See `$docs/directives.md` for more details on image preprocessing. Preprocessing directives must follow any subpath specifier if present. For non-image assets, whatever follows `?` is ignored. Some callbacks noted below (and in `assets.md`) disallow directives (and therefore `?`) in paths entirely.
  - In most cases, accessing a nonexistent asset will throw an error. However, accessing a nonexistent sound or image asset will instead log a warning and load `/assetmissing.png` or `/assetmissing.wav`, respectively, without any frame specifier but *with* any directives.
  - Asset paths in animation configs additionally treat angle brackets (`<` and `>`) as special characters that wrap animation tag specifiers. See below for details. Note that despite this, the characters `<` and `>` are still permitted (but not recommended!) in asset file and directory names.
  - JSON asset configs, but *not* Lua callbacks, may specify a relative path by omitting the first slash; the path is relative to the location of the JSON config in which the relative path is specified.

> **Technical note:** Some technical caveats to what's explained above:
>
> - Actually, `?` and `:` characters are *technically* allowed in "raw" asset paths passed to `assets` callbacks. However, since directories and files with these characters in their names can't be accessed outside of asset preprocessor scripts anyway, there's little reason to bother with this.
> - `assets.add` is the only way to create an asset directory or file named `.` or `..`, since most (or all?) OSes do not allow files or directories with those names to be created.
> - The placeholder image and audio assets for when an asset can't be found can be changed under `"assetsSettings"` in `$executableDirectory/xsbinit.config` (or `$executableDirectory/sbinit.config` on stock Starbound), as follows:
> ```json
> /* xsbinit.config / sbinit.config */
> {
>   // ...
>   "assetsSettings": {
>     "missingImage": "/assetmissing.png", // Raw asset path to the placeholder image.
>     "missingAudio": "/assetmissing.wav" // Raw asset path to the placeholder audio file.
>   }
> }
> ```

### JSON paths

A JSON path may optionally be specified after a colon in a JSON asset path (where it is called a *subpath*), and some callbacks accept a JSON path (marked with a `JsonPath` type) directly. The path string syntax is as follows:

- *`k`:* A JSON object "root" key, where `k` is substituted for the literal key string, without quotes; e.g., `parameters`.
- *`.k`:* A JSON object "subdirectory" key; e.g., `.animationConfig`. Used to access the value associated with a given key inside an object associated with the given "root" or "subdirectory" key. `k` is substituted as for "root" keys above. A path string may *not* begin with a subdirectory key.
- *`[x]`:* A JSON array index specifier, where `x` is either an unsigned integer or `-` (refers to the last index in the array). Used to access the value at the specified array index. Indices begin at `0`, not `1`. Path strings *may* begin with an array index, assuming the "root" is an array (as in many JSON patch files).

Keys and index specifiers may be freely concatenated as in Lua code; e.g., `"actionOnReap[2].action"` accesses the value of the key `"action"` of an object at the third index of the array `"actionOnReap"` at the "root" of the config.

**Note:** The following caveats apply:

- Since `.` and `[` are always parsed as operators, object keys containing any `.` or `[` characters cannot be specified in a JSON path and must instead be directly accessed in Lua.
- In asset paths, `?` always terminates a subpath, and as such, object keys containing `?` characters cannot be specified in an asset path containing a JSON subpath and have to be directly accessed in Lua instead.

Because of this and other considerations, JSON keys containing non-alphanumeric characters should be avoided in Starbound JSON.

### Frame specifiers

A frame specifier may optionally be specified after a colon in an image asset path. A frame specifier may include any character that isn't a null byte (`\0`) or question mark (`?`), but should be restricted to a combination of periods and alphanumeric characters.

If a frame specifier is specified, the game will search for a `.frames` JSON file with the same name as the image (aside from the extension) in the following directories in order, stopping if the file is found:

- The same asset directory as the image file.
- Any asset directory containing the image file's directory, going up directories recursively.
- The asset root.

If a `.frames` JSON file is found and references the given frame specifier, the image will be cropped to the specified `RectU` coordinates before the game uses it. This cropping is done *before* directives are processed.

If the frame specifier is not referenced in the `.frames` file, or no suitable `.frames` file exists, a warning will be logged and the asset `$assets/assetmissing.png`, with no frame cropping, will be used instead; any directives will be applied to that image.

**Note:** Angled brackets (`<` and `>`) are allowed in frame specifiers but should not be used in `.frames` files to avoid interfering with tag replacement (see below).

### Animation tags

Asset paths in animation configs treat angle brackets (`<` and `>`) as special characters that wrap tag specifiers; these wrapped specifiers are called *animation tags*. An animation tag may include any character that isn't a null byte (`\0`) or a right angle bracket (`>`), but should be limited to alphanumeric characters.

The engine replaces animation tags before doing any other path processing under the following circumstances:

  - *`<effectDirectives>`:* In player effects animation configs, replaced with the value of `"effectiveDirectives"` specified in the appropriate `.species` config file for the player's species.
  - *`<color>`:* In all object animation configs, replaced with the value of the object's saved `"color"` parameter.
  - *`<state>` and `<key>`:* In container object animation configs, replaced with the container's current state (`crafting` or `idle`) and the current animation frame number (e.g., `1`, `2`, etc.), respectively.
  - *`<stage>` and `<alt>`:* In farmable object animation configs, replaced with the farmable's current primary and alternate stage names.
  - *Any other tag:* `animator.setPartTag` and `animator.setGlobalTag` (and the equivalents in `playerAnimator`) replace any tag matching the given specifier with the given replacement string, either only for a specific animation part or for the entire animation config.

Tags that aren't replaced are left untouched, potentially allowing angled brackets to remain in the path after tag processing.

## Path types

The following path type names are used throughout this documentation to refer to path strings with differing restrictions:

- **`AssetPath<TaggedImage>`:** A standard asset path where a frame specifier and/or directives are permitted and `< >` tags may be replaced. Only used in JSON animation configs. (`?` and `:` allowed.)
- **`AssetPath<Image, Directives>`:** A standard asset path where an image frame specifier and/or directives are permitted, as detailed above. (`?` and `:` allowed.)
- **`AssetPath<Image>`:** A standard asset path where an image frame specifier is permitted, but *not* directives. (`:`, but *not* `?`, allowed.)
- **`AssetPath<Json>`:** An asset path that allows a JSON subpath but *not* directives. (`:`, but *not* `?`, allowed.)
- **`AssetPath<>`:** An asset path that disallows *both* frame specifiers / subpaths and directives. (Neither `?` nor `:` allowed.)
- **`RawAssetPath`:** A raw asset path where no special meaning is assigned to `?` or `:`. See the *Technical note* above.
- **`FilePath`:** An OS file path as detailed above.

---

The following `root` callbacks are available:

---

#### `List<FilePath>` root.assetSources()

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Returns a list of file paths for all loaded asset sources. If the game loads any assets created by preprocessor scripts, file paths to any asset sources that contained those scripts, with `::onLoad` (for any assets created by on-load scripts) or `::postLoad` (for any assets created by post-load scripts), will be added to this list.

---

#### `List<AssetPath<>>` root.assetsByExtension(`String` extension)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Returns a list of all assets that have a given file extension. Extension matches are case-insensitive; any initial period in the specified extension is optional.

---

#### `Maybe<FilePath>` root.assetSource(`AssetPath<>` assetPath)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Returns the file path to the asset source containing the loaded version of the asset at the specified asset path, or `nil` if no asset exists at that path; passing a path containing a subpath or directives will result in a `nil` return.

If the loaded version of the asset was created by an asset preprocessor script, the path will be followed by `::onLoad` for an asset created by an on-load script or `::postLoad` for an asset created by a post-load script.

---

#### `Maybe<List<FilePath>>` root.assetPatches(`AssetPath<>` assetPath)
#### `Maybe<List<FilePath>>` root.assetPatchSources(`AssetPath<>` assetPath)

> **`root.assetPatches` is only available on xStarbound v3.1.5r1+ and OpenStarbound, while `root.assetPatchSources` is xStarbound-only.**

Returns a list of file paths to any asset sources that contain JSON or Lua patches to the base asset at the specified asset path, or `nil` if no base asset exists at that path; passing a path containing a subpath or directives will result in a `nil` return.

Patches *directly* executed by preprocessor scripts (not via an invocation of `asset.patch` on a patch file) will not be listed.

---

#### `Json` root.assetSourceMetadata(`FilePath` assetSourcePath)

> **Only available on xStarbound.**

Returns the metadata for any asset source whose root directory or `.pak` is at the specified path, `jobject{}` if it lacks a metadata file (called either `.metadata` or `_metadata`), or `nil` if there's no asset source there. An example metadata return value showing all possible keys:

```lua
jobject{
  name = "MyMod",
  includes = jarray{},
  requires = jarray{},
  priority = 1,
  scripts = jobject{
    onLoad = jarray{"/myscript.lua", "/myotherscript.lua"},
    postLoad = jarray{}
  },
  friendlyName = "My Mod",
  version = "v1.0",
  description = "This is my new mod!",
  author = "FezzedOne",
  tags = "Crafting and Building|User Interface",
  steamContentId = "123456789",
  link = "steam://url/CommunityFilePage/123456789",
  permissions = jarray{} -- You may see this in StarExtensions mods.
}
```

Analogous to the preprocessing callback `assets.sourceMetadata` (see `assets.md`). See `$doc/preprocessing.md` for more documentation on asset source metadata files.

---

#### `bool` root.assetExists(`RawAssetPath` assetPath)

> **Only available on xStarbound.**

Returns whether an asset exists at the specified path. Analogous to `assets.exists` (see `assets.md`).

---

#### `Json` root.assetJson(`AssetPath<Json>` assetPath)

Returns the contents of the specified JSON asset file, or if a subpath is specified, the JSON value at that subpath in the asset file.

Throws an error and may log another uncatchable error if the specified asset doesn't exist, the specified asset isn't valid top-level JSON, the path isn't valid, or any specified subpath doesn't exist or isn't valid.

Consider using `root.assetExists`, `root.assetData` and/or `sb.parseJson` to avoid unnecessary error handling.

---

#### `Json` root.assetFrames(`AssetPath<>` assetPath)

Returns the frames specification this image asset file would use, or `nil` if there is no applicable frames file, the specified asset isn't an image, or the asset doesn't exist. A returned frames specification has the following format:

```lua
jobject{
    -- Path to the JSON frame configuration asset used to construct the return value.
    file = "/items/armors/pants.frames",
    
    -- A table of frame aliases where each alias maps to the name of the actual frame it references.
    aliases = jobject{
        ["lay.1"] = "idle.1",
        ["swimIdle.2"] = "swimIdle.1",
        ["swim.5"] = "swimIdle.1",
        ["swim.6"] = "swimIdle.1",
        ["swim.7"] = "swimIdle.1"
    },
    
    -- A list of frames where each frame name maps to a set of RectI coordinates that define the region of
    -- the image to be cropped out for that frame (as with the `?crop` directive`).
    frames = jobject{
        ["jump.3"] = jarray{129, 129, 172, 172},
        ["jump.4"] = jarray{172, 129, 215, 172},
        ["fall.1"] = jarray{215, 129, 258, 172},
        -- Additional frame coordinate mappings would go here.
    }
}
```

Throws an error and may log another uncatchable error if the specified path is invalid.

Analogous to `assets.frames` (see `assets.md`).

---

#### `String` root.assetData(`AssetPath<>` assetPath)

Returns the contents of the specified asset file as a `String` of raw bytes; for a text file, this is the (UTF-8-encoded) text it contains. Throws an error (but doesn't also log an uncatchable error) if the specified asset doesn't exist, or the path isn't valid or contains disallowed components.

Consider using `root.assetExists` to avoid unnecessary error handling.

---

#### `Image` root.image(`AssetPath<Image, Directives>` assetPath)

> **Only available on xStarbound.**

Returns the specified asset file as an `Image` object, if it exists, is a valid image and any frame specifier is valid for it. Otherwise, it logs an uncatchable warning and returns an `Image` object based on `/assetmissing.png` without a frame specifier but with any directives applied. Throws an error if an invalid path is specified.

See `image.md` for information on `Image` object methods. Consider using `root.assetExists` to avoid unnecessary error handling.

---

#### `Image` root.newImage(`Vec2U` size)

> **Only available on xStarbound.**

Creates a new `Image` object with the specified size in pixels. All pixels are initially set to `{0, 0, 0, 0}` [`#00000000`]. 0×0-pixel image objects can be created, but they're of no real use.

See `image.md` for information on `Image` object methods.

---

#### `Maybe<ByteArray>` root.bytes(`AssetPath<>` assetPath)

> **Only available on xStarbound.**

Returns the specified asset as a raw `ByteArray`, if it exists. Throws an error (but doesn't also log an uncatchable error) if the specified asset doesn't exist, or the path isn't valid or contains disallowed components.

See `bytes.md` for information on `ByteArray` object methods.

---

#### `ByteArray` root.newBytes()

> **Only available on xStarbound.**

Creates a raw, blank `ByteArray` of zero length.

See `bytes.md` for information on `ByteArray` object methods.

---

#### `bool` root.saveAssetPathToImage(`AssetPath<Image, Directives>` image, `String` exportFileName, `Maybe<bool>` byFrame)

> **Only available on xStarbound.**

Takes the image asset at the given asset path, crops it to any frame specified, processes any directives and then saves the output to `$storageDir/sprites/$exportFileName.png`, where `$storageDir` is your player/universe storage directory and `$exportFileName` is the `exportFileName` parameter, minus anything before the last directory separator (`/` on Linux; `\` *and* `/` on Windows), if you've left any slashes in there. If `$storageDir/sprites/` does not exist, it will be created for you.

If no base image asset exists at the specified base path or the frame specifier is invalid, the image will be replaced with `$assets/assetmissing.png` and an uncatchable warning will be logged. If any specified directives are invalid, an invisible image will be saved and an uncatchable warning will, again, be logged.

If `byFrame` is `true`, directives are processed on a frame-by-frame basis for those base image assets that have an associated frames file — this is necessary for generated clothing.

This callback is useful for recovering sprites from directive strings. 

**Note:** You should not use `byFrame` with generated directive strings that happen to have one or more non-trivial scaling directives (which aren't just `?scalenearest=1.0`) at the end — pixels may be cropped off — so either remove those scaling directives or "export" them on a frame-by-frame basis by specifying the frame between the base path and directives, then copy out the output files.

---

#### `void` root.exportImage(`Image` image, `String` exportFileName)

> **Only available on xStarbound.**

Exports the given `Image` object as a PNG image to `$storage/sprites/$exportFileName.png`, where `$storage` is your player/universe storage directory and `$exportFileName` is the file name you specified, minus anything before the last directory separator (`/` on Linux; `\` *and* `/` on Windows), if you've left any slashes in there. If `$storageDir/sprites/` does not exist, it will be created for you.

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

Returns the pixel dimensions of the specified image asset. Throws an error if neither the image nor `/assetmissing.png` can be loaded.

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

Generates an item from the specified descriptor, level and seed and returns a new item descriptor for the resulting item, or `nil` if a null item descriptor is specified. A null item descriptor is one which is `nil` or whose name is `""`.

> **Note:** xStarbound v3.0.1 fixes a stock Starbound bug where any `null` item descriptor (i.e., empty treasure slots) returned by this callback immediately causes a segfault; instead, on xStarbound, an explicit `nil` is returned for any such item.

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

> **Note:** xStarbound v3.0.1 fixes a stock Starbound bug where any `null` item descriptors (i.e., empty treasure slots) returned by this callback immediately cause a segfault; instead, on xStarbound, explicit `nil`s are returned for any such items.

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

Returns `true` if a tech with the specified name exists or `false` otherwise.

---

#### `String` root.techType(`String` techName)

Returns the type (tech slot) of the specified tech — one of `"head"`, `"body"` or `"legs"` — or `"head"` if the tech doesn't exist.

**Note:** On stock Starbound, the player is kicked to the menu on the client or the server world shuts down on the server when a nonexistent tech name is passed to this callback.

---

#### `Json` root.techConfig(`String` techName)

Returns the JSON configuration for the specified tech, or `nil` if the tech doesn't exist.

**Note:** On stock Starbound, the player is kicked to the menu on the client or the server world shuts down on the server when a nonexistent tech name is passed to this callback.

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

Loads a configured behaviour, initialises it and returns the behaviour state as a userdata `BehaviorState` object (see below). Explanation of the parameters:

- `context` is the Lua context to pass to behaviour scripts running in the new `BehaviorState` context; in most cases, this should be `_ENV` or perhaps `_G`. Pass `{}` to start with a blank Lua context (but note that behaviour scripts won't be able to access any callbacks!).
- `config` can be either the `String` name of a behaviour tree, or an entire `JsonObject` behaviour tree configuration to be built.
- `parameters` may contain overrides for parameters for the behaviour tree.
- `blackboard` is an optionally specified existing `Blackboard` object (see below) to use for running the behaviour.

---

#### `Json` root.getConfiguration(`String` key)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Gets the value of the specified key in `xclient.config`. Returns `nil` if the key doesn't exist. Will log a warning and return `nil` if any attempt is made to get the value of `"title"`, since that may contain server login info.

---

#### `Json` root.getConfigurationPath(`String` path)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Gets the value at the specified JSON path in `xclient.config`. Uses the same path syntax used in JSON patches. Returns `nil` if nothing exists at the specified path. Will log a warning and return `nil` if any attempt is made to get the value of `"/title"` or anything inside it, since that may contain server login info.

---

#### `Json` root.setConfiguration(`String` key)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Sets the value of the specified key in `xclient.config` to the specified value, returning the newly set value if successful. Returns `nil` if the key doesn't exist. Will log a warning and return `nil` if any attempt is made to set the value of `"safeScripts"`, for obvious reasons.

---

#### `Json` root.setConfigurationPath(`String` path)

> **Only available on xStarbound, OpenStarbound and StarExtensions.**

Sets the value at the specified JSON path in `xclient.config` to the specified value, returning the newly set value if successful. Uses the same path syntax used in JSON patches. Returns `nil` if nothing exists at the specified path. Will log a warning and return `nil` if any attempt is made to set the value of `"/safeScripts"` or anything inside it, for obvious reasons.

---

## `BehaviorState` objects

`BehaviorState` objects have the following methods. All methods take the object itself as the first argument, so use the `:` syntax.

Example:

```lua
function init()
  self.behavior = root.behavior(_ENV, "monster", jobject{})
end

function update(dt)
  self.behavior:run(dt)
end
```

---

#### `Blackboard` `[BehaviorState]`:blackboard()

Returns the `Blackboard` object (see below) for the behaviour. Will throw an exception if the behaviour has expired.

To create a blank blackboard for use in another `BehaviorState`, create a new valid behaviour state of any kind and then immediately call `:blackboard` on it.

Example:

```lua
function init()
  self.blackboard = root.behavior(_ENV, "monster", jobject):blackboard()
  self.mainBehavior = root.behavior(_ENV, "npc", jobject{}, blackboard)
end

function update(dt)
  self.blackboard:set("vec2", "targetVelocity", jarray{0.0, 5.0 * mcontroller.facingDirection()})
  local currentPosition = self.blackboard:get("position", "currentPosition")
end
```

---

#### `void` `[BehaviorState]`:run(`DeltaTime` dt)

Runs the behaviour for a single tick. Will throw an exception if the behaviour has expired.

---

#### `void` `[BehaviorState]`:clear()

Resets the internal state of the behaviour. Will throw an exception if the behaviour has expired.

---

## `Blackboard` objects

`Blackboard` objects have the following methods. All methods take the object itself as the first argument, so use the `:` syntax. Note that references to `Blackboard` objects work like references to Lua tables, so you can edit a blackboard even after passing it to a `BehaviorState`.

### `NodeParameterType`

`NodeParameterType` is a `String` type name that acts as a "second key" into a blackboard. The type names and their recommended corresponding Lua value types are as specified below.

- `"json"`: Any valid JSON-encodable value.
- `"entity"`: An integer entity ID.
- `"position"`: A `Vec2` representing a world position or similar. E.g., `{1.0, 2.0}`.
- `"vec2"`: Any other `Vec2`. E.g., `{1.0, 2.0}`.
- `"number"`: A float or integer number.
- `"bool"`: A boolean.
- `"list"`: A Lua array; should be JSON-encodable. E.g., `jarray{"foo", "bar", "baz"}`.
- `"table"`: A Lua "object"; should be JSON-encodable. E.g., `jobject{foo = 1, bar = jarray{}, baz = false}`.
- `"string"` A string.

Note that a value assigned to any key/`NodeType` combination may technically be any valid Lua value (aside from `nil`, of course). The above are just recommendations.

> **Note:** Make sure all data stored in a `Blackboard` is JSON-encodable, since internal exceptions will be thrown if it's not. You can test for JSON encodability by checking the first result of `pcall(sb.printJson, valueToTest)`, where `valueToTest` is the value you wish to check; if you get `true`, it's safe to put in a `Blackboard`.

---

#### `LuaValue` `[Blackboard]`:get(`NodeParameterType` type, `String` key)

Gets the value stored in the node whose key and `NodeParameterType` is specified. Returns `nil` if the key/`NodeParameterType` combination isn't set.

---

#### `void` `[Blackboard]`:set(`NodeParameterType` type, `String` key, `LuaValue` value)

Sets the specified key/`NodeParameterType` combination to the specified value. If the specified value is `nil`, clears the specified key/`NodeParameterType` combination. Note that keys of different node types may share the same name; each key/type combination is unique.

**Caveat:** If the specified `NodeParameterType` is a `"number"`, the given number value is assigned to an `{x, y}` array in a new or existing `"vec2"` node with the specified key *in addition to* being assigned to a `"number"` node with the same key. For instance, a value of `2.5` becomes `{2.5, 2.5}` in a `"vec2"` node *and* `2.5` in a `"number"` node with the same key.

---

In addition, `NodeParameterType`-specific getters and setters are available:

### Getters

#### `Json` `[Blackboard]`:getEntity(`String` key)
#### `Json` `[Blackboard]`:getPosition(`String` key)
#### `Json` `[Blackboard]`:getVec2(`String` key)
#### `Json` `[Blackboard]`:getNumber(`String` key)
#### `Json` `[Blackboard]`:getBool(`String` key)
#### `Json` `[Blackboard]`:getList(`String` key)
#### `Json` `[Blackboard]`:getTable(`String` key)
#### `Json` `[Blackboard]`:getString(`String` key)

### Setters

#### `void` `[Blackboard]`:setEntity(`String` key, `Json` value)
#### `void` `[Blackboard]`:setPosition(`String` key, `Json` value)
#### `void` `[Blackboard]`:setVec2(`String` key, `Json` value)
#### `void` `[Blackboard]`:setNumber(`String` key, `Json` value)
#### `void` `[Blackboard]`:setBool(`String` key, `Json` value)
#### `void` `[Blackboard]`:setList(`String` key, `Json` value)
#### `void` `[Blackboard]`:setTable(`String` key, `Json` value)
#### `void` `[Blackboard]`:setString(`String` key, `Json` value)

**Note:** `:setNumber` has the same caveat as `:set` done with a `NodeParameterType` of `"number"`.