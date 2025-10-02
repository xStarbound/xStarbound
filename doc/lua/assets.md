# `assets`

> **Only available on xStarbound and OpenStarbound.**

The `assets` table is available in the following script contexts:

- asset preprocessor scripts
- asset patch scripts

See `root.md` for information on the `RawAssetPath` type.

---

## Invoked calls

The engine does not invoke any functions in preprocessor scripts. However, the following function is invoked by the engine in asset patch scripts, after the script is loaded:

---

#### `Maybe<Json<Top>>` patch(`Json<Top>` original)

#### `Maybe<Image>` patch(`Image` original)

Called when the JSON asset or PNG image file that this Lua script is meant to patch is loaded.

The engine passes the "original" JSON asset or PNG file — which may have already been modified by other JSON and Lua patches and preprocessor scripts — into the function. For image patches, the engine expects either `nil` or an `Image` object to be returned.

If it isn't `nil` or a JSON `null`, the return value will replace the original asset when loaded, barring any further patches. See `$doc/preprocessing.md` for more information on the execution order for JSON and Lua patches.

> **Note:** The game lazy-loads assets whenever possible, so sometimes a patch script will not be called until just before the asset is used for the first time in a game session.
>
> No need to worry — patch scripts are always called in the expected order, although you might not see your patch's status logged before the game shows the title screen.

`Image` objects may be modified using invoked Lua methods. See `image.md` for `Image` methods.

---

## General callbacks

The following `assets` callbacks are available to both preprocessor and patch scripts:

---

#### `List<AssetPath<>>` assets.byExtension(`String` extension)

Returns a list of all loaded assets that have the specified file extension. A leading period is optional — `"lua"` works just as well as `".lua"`. See _Asset file extensions_ below for more details on extensions.

Analogous to `root.assetsByExtension`; see `$docs/lua/root.md` for more details.

---

#### `List<AssetPath<>>` assets.scan(`String` beginOrEnd, `Maybe<String>` end)

If both `beginOrEnd` and `end` are specified, returns a list of all loaded assets whose asset paths _begin_ with `beginOrEnd` and _end_ with `end`.

If only `beginOrEnd` is specified, returns a list of all loaded assets whose asset paths _end_ with `beginOrEnd`. If `beginOrEnd` is a file extension, this is equivalent to `assets.byExtension` called on the specified extension.

---

#### `List<AssetPath<>>` assets.sourcePaths(`FilePath` sourcePath)

Returns a list of paths for all asset files in the specified asset source. Ignores the metadata file.

If invoked in a preprocessor script from a given asset source, files added or removed by that script will _not_ be listed since the new memory asset source isn't yet finalised.

Analogous to `root.assetSourcePaths`; see `$docs/lua/root.md` for more details.

---

#### `List<FilePath>` assets.sources()

> **`assets.sources` is only available on xStarbound. `assets.sourcePaths` is available on xStarbound and OpenStarbound v0.1.9+. The OpenStarbound version has an optional boolean parameter (that returns a metadata map) ignored by xStarbound.**

Returns a list of all asset sources detected by the game, regardless of whether they're loaded. Does not include memory asset sources.

Analogous to `root.assetSources` on xStarbound and `root.assetSourcePaths` on OpenStarbound and StarExtensions; see `$docs/lua/root.md` for more details.

---

#### `Maybe<FilePath>` assets.origin(`RawAssetPath` path)

#### `Maybe<FilePath>` assets.source(`RawAssetPath` path)

> **`assets.source` is only available on xStarbound. `assets.origin` is available on xStarbound and OpenStarbound v0.1.9+.**

Returns the file path to the asset source for a _loaded_ asset.

If the loaded version of the asset was created by an asset preprocessor script, the path will be followed by `::onLoad` for an asset created by an on-load script or `::postLoad` for an asset created by a post-load script.

Changes made by `assets.add` or `assets.remove` are reflected immediately for this callback.

Analogous to `root.assetOrigin` and `root.assetSource`; see `$docs/lua/root.md` for more details.

---

#### `Json` assets.patches(`RawAssetPath` path, [`bool` returnFilePaths])

#### `Json` assets.patchSources(`RawAssetPath` path, [`bool` returnFilePaths])

> **Only available on xStarbound.**

If `returnFilePaths` is `false`, `nil` or unspecified, returns a JSON array listing file paths to all _loaded_ patch sources for the specified asset; if the specified asset doesn't exist or isn't loaded, returns `nil`.

If `returnFilePaths` is `true`, returns a list of asset patches for the specified asset, or `nil` if the asset doesn't exist. The list has the following format:

```lua
jarray{
  jarray{
    -- [1] The filesystem path (`FilePath`) to the asset source supplying the patch.
    "/home/user/.local/share/Steam/steamapps/common/Starbound/assets/SomeMod.pak",
    -- [2] The asset path (`AssetPath<>`) for the patch itself.
    "/client.config.patch"
  },
  ...
}
```

Patches directly executed by a preprocessor script will not be listed. The output immediately reflects changes made by `assets.patch`, even if the memory source containing the patch isn't yet finalised.

Analogous to `root.assetPatches` (if using `returnFilePaths`) and `root.assetPatchSources` (without `returnFilePaths`); see `$docs/lua/root.md` for more details.

---

#### `bool` assets.exists(`RawAssetPath` path)

> **Only available on xStarbound.**

Returns whether any asset exists and is loaded at this path. Any changes made via `assets.add` or `assets.erase` show up immediately for this callback, so it's fine to check if an asset made in the same preprocessor script exists.

**Note** This will return `false` on any asset that exists in the asset sources but isn't yet loaded (as is likely to be the case in on-load preprocessor scripts that load before other mods). You can't forcibly load unloaded assets anyway, so this is a wash.

---

#### `Json` assets.json(`AssetPath<Json>` path)

Returns the contents of the specified JSON asset file, or if a subpath is specified, the JSON value at that subpath in the asset file.

Throws an error and may log another uncatchable error if the specified asset doesn't exist, the specified asset isn't valid top-level JSON, the path isn't valid, or any specified subpath doesn't exist or isn't valid.

Consider using `assets.exists`, `assets.bytes` and/or `sb.parseJson` to avoid unnecessary error handling.

See `root.md` for documentation on JSON subpaths.

---

#### `Image` assets.image(`AssetPath<Image, Directives>` path)

Returns the specified asset file as an `Image` object (see `image.md`), if it exists, is a valid image and any frame specifier is valid for it. Otherwise, it logs an uncatchable warning and returns an `Image` object based on `/assetmissing.png` without a frame specifier but with any directives applied. Throws an error if an invalid path is specified.

An uncatchable warning is also logged if any directives are invalid; the returned image will be invisible.

Consider using `assets.exists` to avoid unnecessary error handling.

---

#### `Json` assets.frames(`AssetPath<>` path)

Returns the frames specification this image asset would use, or `nil` if there is no applicable frames configuration, the specified asset isn't an image, or the asset doesn't exist. A returned frames specification has the following format:

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

---

#### `String` assets.bytes(`AssetPath` path)

Returns the contents of the specified asset file as a `String` of raw bytes; for a text file, this is the (UTF-8-encoded) text it contains. Throws an error if no asset exists at the specified path or the asset isn't yet loaded.

Consider using `assets.exists` to avoid unnecessary error handling.

---

#### `String` assets.rawBytes(`AssetPath` path)

> **Only available on xStarbound.**

Returns the contents of the specified asset file as a raw `ByteArray` (see `bytes.md`). Throws an error if no asset exists at the specified path or the asset isn't yet loaded.

Consider using `assets.exists` to avoid unnecessary error handling.

---

#### `Image` assets.newImage(`Vec2U` size)

> **Only available on xStarbound.**

Creates a new `Image` object (see `image.md` for methods) with the specified size in pixels. All pixels are initially set to `{0, 0, 0, 0}` [`#00000000`]. 0×0-pixel image objects can be created, but they're of no real use.

---

#### `ByteArray` assets.newRawBytes()

> **Only available on xStarbound.**

Creates a raw, blank `ByteArray` (see `bytes.md` for methods) of zero length.

---

#### `Maybe<JsonObject>` assets.sourceMetadata(`FilePath` assetSource)

> **Only available on xStarbound.**

Returns the metadata for any _loaded_ asset source whose root directory or `.pak` is at the specified path, `jobject{}` if it lacks a metadata file (called either `.metadata` or `_metadata`), or `nil` if there's no asset source there. An example metadata return value showing all possible keys:

```lua
jobject{
  name = "MyMod",
  includes = jarray{},
  requires = jarray{},
  substitutes = jarray{},
  priority = 1,
  scripts = jobject{
    onLoad = jarray{"/myscript.lua", "/myotherscript.lua", "/myplutoscript.pluto"},
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

If invoked on a non-finalised memory asset source (see `assets.loadedSources` above), returns `nil` because it isn't finalised yet.

Analogous to the callback `root.assetSourceMetadata` (see `root.md`). See `$doc/preprocessing.md` for more documentation on asset source metadata files.

> _Note:_ This callback was available only in preprocessor scripts prior to xStarbound v3.4.4.1.

---

## Preprocessor-only callbacks

The following `assets` callbacks are available only to preprocessor scripts:

---

#### `List<FilePath>` assets.loadedSources()

> **Only available on xStarbound.**

Returns a list of all _loaded_ asset sources. Includes all memory asset sources loaded _prior_ to the preprocessor scripts for this asset source — i.e., it won't include the memory asset source being worked on by a preprocessor script, since it isn't finalised yet.

**Note:** A memory asset source will not be added when finalised if it ends up containing no added files.

---

#### `void` assets.add(`RawAssetPath` path, `Variant<String, Image, ByteArray, Json>` data)

> _May not currently work properly on OpenStarbound._

Adds the specified asset data to the specified path in a memory asset source based on the working asset source. Note that strings will get converted to UTF-8 text file assets; if the string is valid JSON, it's a valid JSON asset.

In an on-load preprocessor script, the resulting memory asset source will be loaded right after the working asset source in terms of precedence.

In a post-load script, the resulting memory asset source will be loaded after all non-memory sources are loaded, but if multiple post-load memory asset sources are loaded, they use the same precedence order as all the base asset sources with regards to other memory asset sources.

---

#### `bool` assets.erase(`RawAssetPath` path)

Deletes the asset at the specified path, immediately telling the engine that there's no asset there. This works on both base assets and loaded memory assets. Returns whether the asset is deleted.

If all memory assets previously added in a memory asset source associated with this script's working asset source are deleted, such that there are no assets in it by the time the preprocessor script finishes, the memory asset source will not be loaded or added to the list of asset sources because it has nothing in it.

> **Technical note:** If a patch file is added via `assets.patch` and then all files, including the patch file, in the memory asset source are removed so that it is "unloaded", the asset source will technically still be loaded even though it won't be listed by `assets.loadedSources` or `root.assetSources` (see `root.md`). The patch source will still show up when `root.assetPatchSources` (see `root.md`) or `assets.patchSources` is invoked on the patched file though.

---

#### `bool` assets.patch(`RawAssetPath` path, `RawAssetPath` patchPath)

Adds the specified JSON or Lua patch file (`patchPath`) to the list of patches for the specified base asset (`path`). The patch will be loaded after any other "standard" patches for the same asset in the asset source. Returns whether the patch is added.

If the patch file has a `.lua` or `.pluto` extension, it will be added as a Lua/Pluto patch. If it has any other extension, it will be added as a JSON patch.

If the specified patch file is later modified, removed or replaced by any script in the same asset source, it will be modified but _not_ deleted for the purposes of checking patches.

---

#### `Json` root.getConfiguration(`String` key)

> **Only available on xStarbound v4.1.1+.**

Gets the value of the specified key in `xclient.config` (on xClient, even if the script is running server-side) or `xserver.config` (on xServer). Returns `nil` if the key doesn't exist. Will log a warning and return `nil` if any attempt is made to get the value of `"title"`, since that may contain server login info. Analogous to `root.getConfiguration` (see `root.md`).

---

#### `Json` root.getConfigurationPath(`String` path)

> **Only available on xStarbound v4.1.1+.**

Gets the value at the specified JSON path in `xclient.config` (on xClient, even if the script is running server-side) or `xserver.config` (on xServer). Uses the same path syntax used in JSON patches. Will log a warning and return `nil` on any attempt to get the value of `"/title"` or anything inside it, since that may contain server login info. Analogous to `root.getConfigurationPath` (see `root.md`).

---

#### `Json` root.setConfiguration(`String` key)

> **Only available on xStarbound v4.1.1+.**

Sets the value of the specified key `xclient.config` (on xClient, even if the script is running server-side) or `xserver.config` (on xServer) to the specified value, returning the newly set value if successful. Returns `nil` if the key doesn't exist. Will log a warning and return `nil` if any attempt is made to set the value of `"safeScripts"`, for obvious reasons. Analogous to `root.setConfiguration` (see `root.md`).

---

#### `Json` root.setConfigurationPath(`String` path)

> **Only available on xStarbound v4.1.1+.**

Sets the value at the specified JSON path `xclient.config` (on xClient, even if the script is running server-side) or `xserver.config` (on xServer) to the specified value, returning the newly set value if successful. Uses the same path syntax used in JSON patches. Returns `nil` if nothing exists at the specified path. Will log a warning and return `nil` if any attempt is made to set the value of `"/safeScripts"` or anything inside it, for obvious reasons. Analogous to `root.setConfiguration` (see `root.md`).

## Asset file extensions

> **Note:** Only xStarbound recognises `pluto` or `patch.pluto` files.

The following asset file extensions are recognised by the engine:

- Generic asset configs: `config` (the engine looks for and loads specific `config` files in the asset root)
- Pluto/Lua scripts: `pluto`, `lua` (aside from versioning scripts, only loaded if explicitly configured in assets)
- Images: `png`
- Fonts: `ttf`
- Music and audio files: `wav`, `ogg`
- Items: `item`, `liqitem`, `matitem`, `miningtool`, `flashlight`, `wiretool`, `beamaxe`, `tillingtool`, `painttool`, `harvestingtool`, `head`, `chest`, `legs`, `back`, `currency`, `consumable`, `blueprint`, `inspectiontool`, `instrument`, `thrownitem`, `unlock`, `activeitem`, `augment`
- Materials and matmods: `material`, `matmod` (items use the `matitem` extension listed above)
- Liquids: `liquid`
- NPC configs: `npctype`
- Tenant configs: `tenant`
- Objects: `object` (these are also their item configs)
- Vehicles: `vehicle`
- Monsters: `monstertype`, `monsterpart`, `monsterskill`, `monstercolors`
- Plants: `modularstem`, `modularfoliage`, `grass`, `bush`
- Projectiles: `projectile`
- Particles: `particle`
- Player name generator configs: `namesource`
- AI mission configs: `aimission`
- Quest templates: `questtemplate`
- Radio message configs: `radiomessages`
- Spawn type configs: `spawntypes`
- Species: `species`
- Stagehand configs: `stagehand`
- Behaviours: `nodes`, `behavior`
- Biomes: `biome`, `weather`
- Terrain configs: `terrain`
- Treasure configs: `treasurepools`, `treasurechests`
- Codex entries: `codex`
- Collections: `collection`
- Statistics configs: `event`, `achievement`
- Status effect configs: `statuseffect`
- Functions: `functions`, `2functions`, `configfunctions`
- Player tech configs: `tech`
- Damage configs: `damage`
- Dances: `dance`
- Effect source configs: `effectsource`
- Command macros: `macros`
- Recipes: `recipe`
- Bind configs: `binds` (xClient, OpenStarbound or StarExtensions)

The following patch extensions are recognised by the engine during asset preprocessing:

- JSON asset patch files: `patch`, `patch<X>` (where `<X>` is a number between 0 and 9, inclusive)
- Lua asset patch files: `patch.pluto`, `patch.lua` (but `assets.patch` can also make the engine invoke any arbitrary script file as a patch)
- Lua preprocessor files: `pluto`, `lua` (only loaded if explicitly configured)
- Front-loaded Lua preprocessor files: `pluto.frontload`, `lua.frontload` (see `$docs/preprocessing.md`)

Aside from the above extensions recognised by the engine, mod scripts may be set up to recognise other file extensions.
