# `assets`

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

----

#### `List<AssetPath<>>` assets.byExtension(`String` extension)

Returns a list of all loaded assets that have the specified file extension. A leading period is optional — `"lua"` works just as well as `".lua"`. See *Asset file extensions* below for more details on extensions.

----

#### `List<AssetPath<>>` assets.scan(`String` beginOrEnd, `Maybe<String>` end)

If both `beginOrEnd` and `end` are specified, returns a list of all loaded assets whose asset paths *begin* with `beginOrEnd` and *end* with `end`.

If only `beginOrEnd` is specified, returns a list of all loaded assets whose asset paths *end* with `beginOrEnd`. If `beginOrEnd` is a file extension, this is equivalent to `assets.byExtension` called on the specified extension.

----

#### `List<FilePath>` assets.sources()

Returns a list of all asset sources detected by the game, regardless of whether they're loaded. Does not include memory asset sources.

----

#### `Maybe<List<FilePath>>` assets.patchSources(`RawAssetPath` path)

Returns a list of file paths to all *loaded* patch sources for the specified asset; if the specified asset doesn't exist or isn't loaded, returns `nil`.

----

#### `bool` assets.exists(`RawAssetPath` path)

Returns whether any asset exists and is loaded at this path. Any changes made via `assets.add` or `assets.erase` show up immediately for this callback, so it's fine to check if an asset made in the same preprocessor script exists.

**Note** This will return `false` on any asset that exists in the asset sources but isn't yet loaded (as is likely to be the case in on-load preprocessor scripts that load before other mods). You can't forcibly load unloaded assets anyway, so this is a wash.

----

#### `Json` assets.json(`AssetPath<Json>` path)

Returns the contents of the specified JSON asset file, or if a subpath is specified, the JSON value at that subpath in the asset file.

Throws an error and may log another uncatchable error if the specified asset doesn't exist, the specified asset isn't valid top-level JSON, the path isn't valid, or any specified subpath doesn't exist or isn't valid.

Consider using `assets.exists`, `assets.bytes` and/or `sb.parseJson` to avoid unnecessary error handling.

See `root.md` for documentation on JSON subpaths.

----

#### `String` assets.image(`AssetPath<Image, Directives>` path)

Returns the specified asset file as an `Image` object (see `image.md`), if it exists, is a valid image and any frame specifier is valid for it. Otherwise, it logs an uncatchable warning and returns an `Image` object based on `/assetmissing.png` without a frame specifier but with any directives applied. Throws an error if an invalid path is specified.

An uncatchable warning is also logged if any directives are invalid; the returned image will be invisible.

Consider using `assets.exists` to avoid unnecessary error handling.

----

#### `String` assets.bytes(`AssetPath` path)

Returns the contents of the specified asset file as a `String` of raw bytes; for a text file, this is the (UTF-8-encoded) text it contains. Throws an error if no asset exists at the specified path or the asset isn't yet loaded.

Consider using `assets.exists` to avoid unnecessary error handling.

----

#### `String` assets.rawBytes(`AssetPath` path)

Returns the contents of the specified asset file as a raw `ByteArray` (see `bytes.md`). Throws an error if no asset exists at the specified path or the asset isn't yet loaded.

Consider using `assets.exists` to avoid unnecessary error handling.

----

#### `Image` assets.newImage(`Vec2U` size)

Creates a new `Image` object (see `image.md` for methods) with the specified size in pixels. All pixels are initially set to `{0, 0, 0, 0}` [`#00000000`]. 0×0-pixel image objects can be created, but they're of no real use.

----

#### `ByteArray` assets.newBytes()

Creates a raw, blank `ByteArray` (see `bytes.md` for methods) of zero length.

----

## Preprocessor-only callbacks

The following `assets` callbacks are available only to preprocessor scripts:

----

#### `List<FilePath>` assets.loadedSources()

Returns a list of all *loaded* asset sources. Includes all memory asset sources loaded prior to the preprocessor scripts for this asset source — i.e., it won't include the memory asset source being worked on by a preprocessor script, since it isn't finalised yet.

**Note:** A memory asset source will not be added when finalised if it ends up containing no added files.

----

#### `Maybe<JsonObject>` assets.sourceMetadata(`FilePath` assetSource)

Returns the metadata for any *loaded* asset source whose root directory or `.pak` is at the specified path, `jobject{}` if it lacks a metadata file (called either `.metadata` or `_metadata`), or `nil` if there's no asset source there. An example metadata return value showing all possible keys:

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

If invoked on a non-finalised memory asset source (see `assets.loadedSources` above), returns `nil` because it isn't finalised yet.

Analogous to the post-preprocessing callback `root.assetSourceMetadata` (see `root.md`). See `$doc/preprocessing.md` for more documentation on asset source metadata files.

----

#### `void` assets.add(`RawAssetPath` path, `Variant<String, Image, ByteArray, Json>` data)

Adds the specified asset data to the specified path in a memory asset source based on the working asset source. Note that strings will get converted to UTF-8 text file assets; if the string is valid JSON, it's a valid JSON asset.

In an on-load preprocessor script, the resulting memory asset source will be loaded right after the working asset source in terms of precedence.

In a post-load script, the resulting memory asset source will be loaded after all non-memory sources are loaded, but if multiple post-load memory asset sources are loaded, they use the same precedence order as all the base asset sources with regards to other memory asset sources.

----

#### `bool` assets.erase(`RawAssetPath` path)

Deletes the asset at the specified path, immediately telling the engine that there's no asset there. This works on both base assets and loaded memory assets. Returns whether the asset is deleted.

If all memory assets previously add in a memory asset source associated with this script's working asset source are deleted such that there are no assets in it by the time the preprocessor script finishes, the memory asset source will not be loaded or added to the list of asset sources because it has nothing in it.

----

#### `bool` assets.patch(`RawAssetPath` path, `RawAssetPath` patchPath)

Adds the specified JSON or Lua patch file (`patchPath`) to the list of patches for the specified base asset (`path`). The patch will be loaded after any other "standard" patches for the same asset in the asset source. Returns whether the patch is added.

If the patch file has a `.lua` extension, it will be added as a Lua patch. If it has any other extension, it will be added as a JSON patch.

If the specified patch file is later modified, removed or replaced by any script in the same asset source, it will be modified but *not* deleted for the purposes of checking patches.

----

## Asset file extensions

The following asset file extensions are recognised by the engine:

- Generic asset configs: `config` (the engine looks for and loads specific `config` files in the asset root)
- Lua scripts: `lua` (aside from versioning scripts, only loaded if explicitly configured in assets)
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
- Lua asset patch files: `patch.lua` (but `assets.patch` can also make the engine invoke any arbitrary script file as a patch)
- Lua preprocessor files `lua` (only loaded if explicitly configured)

Aside from the above extensions recognised by the engine, mod scripts may be set up to recognise other file extensions.