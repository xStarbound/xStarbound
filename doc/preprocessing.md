# Asset preprocessing

This file documents the precedence rules for loading asset sources, the ordering rules for loading asset patches and xStarbound's available JSON patch operations, along with the options that can be specified in them.

----

## Asset source loading precedence rules

The following rules apply, in order, for the precedence of loaded asset sources:

- Any asset source specified by internal name in any other source's `"includes"` or `"requires"` is loaded *before* any sources that include this source in their `"includes"` or `"requires"`.
  - The engine attempts to order recursive "includes" in such a way that "included" sources always get loaded before the sources that "include" them. If there is a circular "include" order, the engine will throw an error stating this and shut down.
- After any reordering above, asset sources are loaded in order of priority — an arbitrary floating-point value specified under the `"priority"` key in the source's metadata file — from lowest to highest.
  - If no priority is specified in a given source's metadata file, it is assumed to be `0.0`.
  - *Example:* Assume Starbound is loading four modded asset sources:
    - Mod A, with a priority of `5.0`, but it "includes" Mod C.
    - Mod B, with a priority of `6.0`.
    - Mod C, with a priority of `7.0`.
    - Mod D, with a priority of `8.0`.
  - The mods are loaded in this order: C, A, B, D. Since Mod C is "included" by Mod A, it's loaded before Mod A regardless of priority.
- After any reordering above, asset sources are loaded in alphabetical order based on their full filesystem paths, dependent on your OS's configured locale for filesystem ordering.
  - *Example:* After "includes" and priority ordering, for mods with Chinese file or directory names in the same directory, Starbound will load them in one order on a system with a `zh_CN` (mainland Mandarin Chinese) locale, and in a different order on a system with an `en_US` (American English) locale.
- After any reordering above, any on-load memory asset sources created by on-load preprocessor scripts are loaded right after each associated base sources containing the executed scripts.
- Lastly, any post-load memory asset sources created by post-load preprocessor scripts are loaded *after everything else*, but using the same precedence ordering *with respect to themselves* as the base asset sources loaded before them.

----

## Asset patch load ordering

All per-source sets of asset patches are loaded in the same order that the asset sources themselves are loaded. Any patch for an asset file that doesn't exist or isn't yet loaded will simply not be executed; no errors will be logged unless the patch is added via `assets.patch` in a preprocessor script.

For each loaded asset source, patches for a given JSON asset file are executed in the following order, where `$asset` is the asset file being patched:

1. `$asset.patch`
2. `$asset.patch.lua`
3. `$asset.patch.pluto`
4. `$asset.patch0`
5. `$asset.patch1`
6. `$asset.patch2`
7. `$asset.patch3`
8. `$asset.patch4`
9. `$asset.patch5`
10. `$asset.patch6`
11. `$asset.patch7`
12. `$asset.patch8`
13. `$asset.patch9`
14. Any patches added via `assets.patch` in on-load preprocessor scripts, in the order they are added. Patches with a `.pluto` or `.lua` extension are considered Pluto/Lua scripts; otherwise they are considered JSON patches.
15. Ditto, but for post-load preprocessor scripts.

Also, for each loaded asset source, patches for a given PNG image asset file are executed in the following order, where `$asset.png` is the PNG asset file being patched:

1. `$asset.png.patch.lua` 
2. `$asset.png.patch.pluto`
3. Any patches added via `assets.patch` in on-load preprocessor scripts, in the order they are added. Patches are Pluto/Lua scripts and must have a `.pluto` or `.lua` extension in order to take effect.
4. Ditto, but for post-load preprocessor scripts.

**Note:** Because capital letters are sorted before lower-case ones when asset files are sorted, a `.patch.Pluto` file with capital letters in its extension will be executed before any `.patch.lua` file whose extension is fully lower-case, among other things.

**xStarbound note:** If a Pluto/Lua patch throws an unhandled error, any patch test fails, or any patch returns anything that isn't valid top-level JSON, that patch is skipped on xStarbound v3.4.2+, rather than sometimes invalidating the entire asset file. Such errors are still logged on xStarbound, as a matter of course.

----

## Metadata files

Any asset source may have a JSON `_metadata` or `.metadata` file at its root, containing optional instructions and stipulations for the asset preprocessor. The metadata it contains must be in the following format (in JSON):

```json
{
  "name": "MyMod", // The asset source's internal name; should be a string. Used for `"includes"`
  // and `"requires"` checks.
  "includes": [], // A list of sources that must be fully loaded (aside from post-load scripts)
  // prior to loading this source. Sources whose names match `"includes"` entries do not need to
  // be present for this source to load, but the game will rearrange load order such that any
  // "included" sources get loaded before this source.
  "requires": [], // A list of sources that *must* be present and fully loaded (aside from any
  // post-load preprocessor scripts) prior to loading this source. If any sources whose names are
  // specified in `"requires"` are not found, the game will shut down. The game will rearrange
  // load order to load any "required" sources before this source.
  "priority": 1, // The mod's priority value, as any valid JSON number.
  "scripts": { // Asset preprocessor scripts.
    "onLoad": ["/myscript.lua", "/myotherscript.lua"], // Scripts to executed upon first loading
    // this asset source.
    "postLoad": [] // Scripts to execute *after* loading all asset sources.
    // Paths to scripts must be absolute raw asset paths.
    // Individual scripts are executed in the order specified in the respective array.
  },
  "friendlyName": "My Mod", // The asset source's "friendly" name. Shown on the mods screen
  // instead of the asset source's internal name if present.
  "version": "v1.0", // The asset source's version string. Shown on the mods screen.
  "description": "This is my new mod!", // The asset source's description string. Shown on the
  // mods screen.
  "author": "FezzedOne", // The asset source's author(s) string. Shown on the mods screen.
  "tags": "Crafting and Building|User Interface", // The asset source's Steam tags listed as a
  // pipe-separated string. The Workshop mod uploader automatically sets `"tags"` based on the
  // selected tags in its interface.
  "steamContentId" : "123456789", // An optional Steam Workshop content ID. Added by the Steam
  // Workshop uploader automatically. If the mod is loaded from the Workshop, this ID is used
  // internally by the Workshop button.
  "link": "steam://url/CommunityFilePage/123456789", // A URL for the mod's homepage, whatever
  // it is. A Steam link to the mod's Steam Workshop page is added automatically by the
  // Workshop mod uploader. The link can be copied from the mods screen.
  "permissions": [] // Required in order to use certain callbacks on StarExtensions. Ignored by
  // xStarbound.
}
```

All JSON keys are optional.

**Warning:** If a non-Workshop asset source lists a Workshop source in `"requires"`, the game will shut down before the required Workshop source is loaded, since the game loads Workshop sources *after* fully processing all other sources. Because of this, `"includes"` is highly recommended instead of `"requires"` whenever Workshop mods might be involved.

To avoid "asset doesn't exist" errors in patches, consider using `assets.exists` in your preprocessor script.

**Technical notes:** Some minor technical notes:

- The `"name"` specified in `"metadata"` may technically be any valid JSON value, but mods cannot "require" or "include" a mod whose internal name is not a string.

- You might have noticed that preprocessor scripts are executed *twice* if the game loads any Workshop mods. Don't worry though — the slate is wiped clean before the second execution. Preprocessor scripts are also re-executed on any `/reload` or `/serverreload`.

----

## JSON patch structure

A valid JSON patch is either 1) an array containing any number of patch operation objects or patch arrays recursively containing any number of patch operation objects or patch arrays, and so on, or 2) an object comprising a Koala-build-style JSON merge patch.

Example of a patch array:

```json
[
    {
        "op": "replace",
        "path": "/foo/hello/bye",
        "value": "adios"
    }
    [
        { // This test operation scopes only to within the inner brackets.
            "op": "test", 
            "path": "/foo/bar",
            "value": 5 // The value at `"foo"` → `"bar"` must match this value.
        },
        { // Ditto.
            "op": "test", 
            "path": "/foo/bean",
            "inverse": true,
            "value": 5 // The value at `"foo"` → `"bean"` must *not* match this value.
        },
        { // This operation will only happen if both tests succeed.
            "op": "add", 
            "path": "/foo/baz",
            "value": 10
        },
        { // Ditto.
            "op": "replace", 
            "path": "/foo/bean",
            "value": 15
        },
    ],
    { // This patch is not affected by the result of the above test operation.
        "op": "replace",
        "path": "/baz", // Path to an array.
        // Replaces an array element containing at least the specified key-value pairs.
        "search": { 
            "someKey": true,
            "someOtherKey": false
        },
        "value": {
            "someKey": false, 
            "anotherKey": true, 
            "someOtherKey": false
        },
        "exact": false
    }
]
```

Example of a merge patch with the same result:

```json
{
    "foo": { 
        "baz": 10, // Adds `"baz"`.
        "bean": 15, // Replaces the value of `"bean"`.
        "hello": { // Recursively merges objects.
            "bye": "adios" // Replaces the value of `"bye"`.
        }
    },
    "baz": [ // Recursive merging stops at arrays. 
        { // So the entire array needs to be replaced.
            "someKey": false,
            "anotherKey": false,
            "someOtherKey": false 
        },
        {
            "someKey": false,
            "anotherKey": true,
            "someOtherKey": false 
        },
    ]
}
```

Base JSON for these examples:

```json
{
    "foo": {
        "12": true,
        "bar": 5,
        "bean": 2,
        "can": 6,
        "blarg": 10,
        "blorg": 18,
        "hello": {
            "bye": "bye bye",
            "greetings": "greeted",
        }
    },
    "baz": [
        {
            "someKey": false,
            "anotherKey": false,
            "someOtherKey": false 
        },
        {
            "someKey": true,
            "anotherKey": false,
            "someOtherKey": false 
        },
    ]
}
```

### JSON patch operation execution order

JSON patch operations are executed sequentially, in the order they are defined.

If an operation fails within its scope (determined by the deepest nested square brackets containing it), no further operations within that scope will be executed.

Each operation immediately modifies the base JSON asset. Specifically, each operation works on the JSON value that may have been altered by the preceding operation. This means that JSON operations can interact with values introduced by earlier operations in the same patch file.

----

## JSON merge operations

> **Available only on OpenStarbound and xStarbound.**

A JSON merge operation works as follows, where "base" is the base value and "merger" is the value to merge:

1. If *both* the base and merger are objects, do the following for every key-value pair in the merger object:
   1. If a key does not exist in the base but does exist in the merger, add the key-value pair from the merger.
   2. If a matching key exists, recursively execute a merge operation on its value, going through the steps above as if the value were the base.
2. If the merger is `null`:
   - If `"nulling"` is true, return the merged `null`.
   - Otherwise, return the base.
3. If neither of the above criteria applies, return the merger.

As a result of this, a JSON merge patch on an array is equivalent to entirely replacing it with the specified value, even if the array contained objects — the operation can't recursively get through the "array barrier" to them.

`"nulling"` is always false for a top-level Koala-style merge patch, which means such patches cannot remove keys.

> On OpenStarbound, `"nulling"` isn't available as an option and is effectively always `true`.

**Note:** This is why JSON merge patches and patch arrays can use the same patch file extensions — a merge patch on a top-level array basically means replacing the entire thing. That can be done with a replacement asset, making top-level array merge patches completely redundant.

----

## JSON patch operation parameters

All xStarbound JSON patch operations support the following parameters:

- **`"op"`:** The string value at this *required* key specifies the type of operation to execute.

- **`"path"`:** The value at this *required* key is the path to the JSON value to search, modify or test. The syntax is as follows:

  - *`/k`:* A JSON object key specifier, where `k` is substituted for the literal key string, without quotes; e.g., `parameters`.

  - *`/n`:* A JSON array index specifier, where `x` is either an usigned integer or `-` (refers to the last index in the array). Used to access the value at the specified array index. Indices begin at `0`, not `1`.

  Due to apparent syntactic ambiguity, specifiers are parsed as follows:

  - As an integer index if an array is found at the path leading to it.
  - As a string key if an object is found at the path leading to it.

  Because of this, there is actually no ambiguity in the syntax as long as the base JSON is available for reference.
  
  > *Example:* Using the example base JSON above, the path `"/foo/12"` refers to the value of the key `"12"` in the `"foo"` object, while `"/baz/1"` refers to the second element in the `"baz"` array.
  
  The escape sequences `~0` and `~1` are parsed as forward slash (`/`) and tilde (`~`) "literals", respectively, allowing access to values at or inside object keys containing forward slashes. This mean any possible JSON key or index is fully accessible in patch arrays.
  
  Keys and index specifiers may be freely concatenated; e.g., `"/actionOnReap/2/action"` accesses the value of the key `"action"` of an object at the third index of the array `"actionOnReap"` at the "root" of the config.

  If only `"/"` is specified, the top-level object or array is what will be modified.
 
- **`"search"` [xStarbound and OpenStarbound only]:** If this *optional* key is present and the base JSON value at the specified path is an array, the array will be searched for the *first* base subvalue matching the specified search value at this key. The precise matching behaviour depends on the value of `"exact"`:

  - `true`: The base subvalue must *exactly* match the search value, including the ordering of elements in nested arrays.

  - `false` or key is not present: The base subvalue must *partially* match the search value, meaning the following:

    - All key-value pairs and/or array elements in the search value must be present and match in the base subvalue (but *not* vice versa), either directly or recursively for nested objects and arrays. 
    - The ordering of nested array elements does *not* matter for the comparison. 
    - If the search value is not an object or array, the base subvalue must exactly match the search value.
  
  If a matching value is found, this will be the value to be modified or tested against, not the array containing it. The patch will fail if no matching value is found.

  > *Example:* `[5, 4, 3]` partially matches `[1, 2, 3, 4, 5, 6]`, but *not* `[9, 8, 4, 3, 2, 6]`, because in the former, all specified elements are present regardless of order, but in the latter, there is no element `5`.

- **`"exact"` [xStarbound only]:** If `"search"` is present, this value at this *optional* key controls whether the matching behaviour is exact or partial. See above.

xStarbound supports the following JSON patch operations:

- **`"test"`:** Applies a test on the specified `"value"` or `"search"` value. If the test fails, any *subsequent* operations within this test's scope — i.e., within the most deeply nested set of brackets containing this operation — are skipped. The precise test depends on the presence and values of the `"search"`, `"value"` and `"inverse"` parameters:

  | `"inverse"`?           | `"value"` present                                                                | `"search"` present                                                        | Neither present                               |
  | ---------------------- | -------------------------------------------------------------------------------- | ------------------------------------------------------------------------- | --------------------------------------------- |
  | `false` or unspecified | Succeeds if the base value at `"path"` matches `"value"`.                        | Succeeds if the `"search"` on the array at `"path"` succeeds (see above). | Succeeds if any value is present at `"path"`. |
  | `true`                 | Succeeds if the base value at `"path"` doesn't exist or doesn't match `"value"`. | Succeeds if the `"search"` on the array at `"path"` fails.                | Succeeds if no value is present at `"path"`.  |

  Additionally, if `"inverse"` is `true`, an inability to traverse to the specified path is always a success; otherwise, an inability to traverse is always a failure.

  This operation has one specific parameter:

  - *`inverse`:* An *optional* boolean controlling whether this is an inverse test.

  If a test fails, a debug message is logged only if debug logging is enabled. To enable debug logging, pass `-loglevel debug` to xClient or xServer when starting either of them.

- **`"remove"`:** If `"search"` is specified and the search succeeds, this operation attempts to remove the found array element. 
  
  If `"search"` is not specified, this operation attempts to remove the key-value pair or array element at the given path.

  Any traversal error logs a warning.

- **`"add"`:** If `"search"` is specified and the search succeeds, this operation attempts to insert the `"value"` in a position right after the found array element. E.g., if the found element happens to be the fifth one in the array, the `"value"` is inserted as the new sixth value before the old sixth value.
  
  If `"search"` is not specified, this operation attempts to add a key-value pair or array element at the given path, using the last key or element specification in the path as the key or index to add.

  Any traversal error up until the last specifier in the path logs a warning.

  > *Example:* An `"add"` operation with a path of `"/foo/bar"` adds a key `"bar"` under the object `"foo"`; `"bar"` will have the value of `"value"`.

- **`"replace"`:** If `"search"` is specified and the search succeeds, this operation attempts to replace the found array element with the `"value"`.

  If `"search"` is not specified, this operation attempts to replace the key-value pair or array element at the given path with the `"value"`.

  Any traversal error logs a warning.

- **`"move"`:** If `"search"` is specified and the search succeeds on the `"from"` path, this operation attempts to move the found array element to the `"path"` path as if it were an `"add"` operation on `"path"`.

  If `"search"` is not specified, this operation attempts to move the key-value pair or array element at the given `"from"` path to the `"path"` path as if it were an `"add"` operation on `"path"`.

  Any traversal error logs a warning.

  This operation has one specific parameter:

  - *`"from"`:* The *required* source path specifier string.

- **`"copy"`:** Like `"move"`, but does not remove the source key-value pair or array element.
  
  This operation has one specific parameter:

  - *`"from"`:* The *required* source path specifier string.

- **`"merge"` [xStarbound and OpenStarbound only]:** If `"search"` is specified and the search succeeds, this operation attempts to execute a JSON merge on the found array element (as the base) with the `"value"` (as the merger). See *JSON merge operations* above for more details.

  If `"search"` is not specified, this operation attempts to execute a JSON merge on the value of the key-value pair or array element (as the base) at the given path with the `"value"` (as the merger).

  Any traversal error logs a warning.

  - *`"nulling"` [xStarbound only]:* An *optional* boolean specifying whether to allow `null`s in the merger to overwrite base values. If unspecified, `null`s won't be allowed. [Assumed `true` by default on OpenStarbound.]
