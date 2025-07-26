# Lua on xStarbound

The following callbacks and deviations from standard Lua 5.3 are shared across all Lua scripts on xStarbound.

## Function specifications and basic Lua types

Due to the particular requirements and caveats of Starbound callbacks, all callbacks and functions are described with C++-style specifications. The following C++-like type names are used throughout this document instead of standard Lua typenames.

- **`void`, `LuaNil`:** A Lua nil.
- **`JsonNull`:** A JSON `null`; is considered "falsy", not "truthy", but is also considered not equal to `nil` and `false`.
  - This is the value of the `null` and `json.null` values available in every context. xStarbound accepts this value in addition to explicit nils in all contexts where JSON values are accepted or required, including in JSON objects and JSON arrays. This allows direct assignment of `null`s when creating objects or arrays with `jobject` or `jarray`.
  - _Technical note:_ This is a Lua light userdata value, but xStarbound's Lua VM has been modified to treat this value as "falsy" for modders' convenience. For compatibility with existing mods that use modified Lua JSON libraries, xStarbound still _returns_ explicit nils from callbacks in all cases.
- **`bool`:** A Lua boolean.
- **`LuaNumber`:** Any Lua number; not implicitly converted. Technically a `Variant<double, uint64_t>` (see `Variant` below). Subtypes:
  - **`double`:** A Lua floating-point number; i.e., a 64-bit floating-point number (or "float" for short). This is the default type for any Lua number that can't be represented as a 64-bit signed integer.
  - **`float`:** A 32-bit float.
  - **`int`:** A Lua integer; `type` will return `"number"` on these. Technically a 64-bit signed integer. Used where the exact integer type is unimportant to normal use of a callback or function.
  - **`EntityId`:** Used for in-game entity IDs, which should be treated differently from standard numbers. A 32-bit signed integer.
  - **`DungeonId`:** Used for in-game dungeon IDs, which should be treated differently from standard numbers. Technically a 16-bit _unsigned_ integer, but the «meta» dungeon IDs take useful advantage of integer underflow.
  - **`int64_t`, `uint64_t`:** A 64-bit signed or unsigned integer, respectively.
  - **`int32_t`, `uint32_t`:** A 32-bit signed or unsigned integer, respectively.
  - **`int16_t`, `uint16_t`:** A 16-bit signed or unsigned integer, respectively.
  - **`int8_t`, `uint8_t`:** An 8-bit signed or unsigned integer, respectively.
  - **`size_t`:** A Lua integer implicitly converted to or from a C++ `size_t`. This is equivalent to `uint64_t` on 64-bit builds (xStarbound is 64-bit-only) or `uint32_t` on 32-bit builds.
- **`String`:** A Lua string. Subtypes:
  - **`Directives`:** A string consisting of any number (or none) of valid image processing directives. See `$doc/directives.md` for more info.
  - **"Enum" types:** Note that many "enum"-style types are cases where callbacks return or accept only a limited number of Lua strings. These are noted where relevant.
  - **`AssetPath`, `RawAssetPath` and `FilePath`:** See `root.md` for more details on these types and their subtypes.
- **`LuaTable`:** A Lua table. Subtypes:
  - **`Vec2F`, `Vec2D`, `Vec2I` and `Vec2U`:** An array of _two_ elements — 32-bit floats, 64-bit floats, 32-bit unsigned integers and 32-bit signed integers, respectively. `Vec2F`s are most commonly used for positions where subpixel or sub-tile positioning does matter, `Vec2I`s are used for world positions where sub-tile positioning is irrelevant and `Vec2U`s are used for pixel positioning in images and panes.
  - **`Vec3F`, `Vec3D`, `Vec3I`, `Vec3U` and `Vec3B`:** An array of _three_ elements — 32-bit floats, 64-bit floats, 32-bit unsigned integers, 32-bit signed integers and 8-bit unsigned integers (i.e., `0` to `255`), respectively. `Vec3F`s are used for percentage-based RGB colours (e.g., in light sources), `Vec3U`s are used for celestial coordinates and `Vec3B`s are used for exact RGB colours.
  - **`RectF`, `RectD`, `RectI`, `RectU` and `Vec4B`:** An array of _four_ elements — 32-bit floats, 64-bit floats, 32-bit unsigned integers, 32-bit signed integers and 8-bit unsigned integers (i.e., `0` to `255`), respectively. `RectF`s are used for in-world "bound boxes", `RectI`s are used for world tile "bound boxes" and celestial regions, and `Vec4B`s are used for exact RGBA colours.
  - **`List<...>`:** An array containing any number (including zero) of elements of the type specified within the angle brackets. One common subtype has an alias:
  - **`PolyF`, `PolyD`, `PolyI` and `PolyU`:** Various arrays of `Vec2`-type elements, often defining polygons, hence the name. These are `List<Vec2F>`, `List<Vec2D>`, `List<Vec2I>` and `List<Vec2U>`, respectively. `PolyF` is used for collision polys and `PolyI` for object spaces (as a list of relative coordinates occupied by the object, not a polygon).
    - **`StringList`:** A `List<String>`, of course.
  - **`JsonArray`:** A Lua table with only unsigned integer keys; use Lua's "array" notation to save typing. Due to how Lua treats nil values, JSON arrays should be created with `jarray`; see below for how to assign or remove explicit `null` values. Indicated by `jarray{...}` within Lua examples.
  - **`JsonArray<...>`:** Same as above, except that all table values must be of the specified type. For instance, a `JsonArray<unsigned>` is an array where all values must be unsigned integers.
  - **`JsonObject`:** A Lua table with only string keys. Due to how Lua treats nil values, JSON objects should be created with `jobject`; see below for how to assign or remove explicit `null` values. Indicated by `jobject{...}` within Lua examples.
  - **`JsonObject<...>`:** Same as above, except that all table values must be of the specified type. For instance, a `JsonObject<JsonArray>` requires all its keys to have `JsonArray` values.
  - **`LuaCallbacks`:** Used only by `interface.bindRegisteredPane` (see `interface.md`). Technically a Lua table containing callback functions, but should really be treated like a userdata object (aside from the "methods" not taking the table itself as their first argument).
- **`LuaFunction`:** A Lua function.
  - A Lua function with a specific return value and arguments is denoted with `LuaFunction<Return...(Args...)>`, where `Return` is the type(s) of the return value(s) and `Args...` is the type(s) of any arguments, all comma-separated; if no argument types are specified, no arguments are passed or used.
- **`LuaThread`:** A Lua thread.
- **`LuaUserdata`:** A Lua userdata object.
  - **Note:** These are returned by several Starbound callbacks and are used for their methods, which are called as if the object were a table containing function values. The first argument of userdata methods in Starbound is always the userdata object itself, so the special `:` syntax should be used to save space and typing, and as such, is used throughout this documentation. Specific userdata types are described in the documentation where relevant.
- **`LuaLightUserdata`:** A Lua light userdata object. Technically returned by `require` or any other Starbound callback if the Lua instruction or recursion limit is reached, but hitting those limits immediately stops script execution anyway. Aside from metatable smuggling, the only use for these is to check their equality to see if two different loaded scripts hit the same type of limit.
  - **Technical stuff:** Light userdata values are technically pointers that can only be compared in Lua. The only pointers ever returned are both to safe static variables though (`int`s that are `0`). The potential light userdata return from `require` is specifically called out because different engine code is used.
- **`LuaValue`:** Any Lua value.
- **`Variant<...>`:** An argument or return value that can be any one of the types specified in a comma-separated list within the angle brackets. Can include variadic types, in which case one or more arguments or return values of _only_ the specified variadic type(s) are accepted or returned.
- **`Maybe<...>`:** An argument or return value that can be either the single type specified within the angle brackets or `nil`. Equivalent to `Variant<void, ...>`, where `...` is the other type.
- **`Json`:** This is a `Variant<void, bool, Number, String, JsonArray, JsonObject>`, and represents valid JSON encoded in Lua. Most callbacks and functions that take this type expect a `JsonObject` with specific keys, but internally allow any valid JSON value; any caveats are noted in the documentation for specific callbacks and functions.
- **`Json<Top>`:** This is a `Variant<JsonObject, JsonArray>` — JSON objects and arrays are the only valid top-level JSON types.
- **`ItemDescriptor`:** A valid item descriptor. Can be any of the following:
  - A `nil`. This is used for an empty item slot or a "null" item.
  - A `String` item name. Has a default item count of 1 and no specified parameters.
  - An array of the format `jarray{name, count, parameters}`, where:
    - `name` is the `String` internal name of the item.
    - `count` is a `size_t` item count, `nil` or unset (defaulting to 1)
    - `parameters` is either a `JsonObject`, `nil` or unset (defaulting to none).
  - An object of the format `jobject{name = name, count = count, parameters = parameters}`; see above.
- **`T`, `T1`, `T2`, `T3`, etc.:** Generic types. Their meaning is that any matching type specifiers must have the same type(s), whatever they are.
  - _Example:_ A function that takes a `T1` and returns another `T1` returns something that has the same type as what was passed in. If the function returns a `T2` but still takes a `T1`, no type match is required. If a function takes two additional `T3` arguments, those two arguments must have a matching type, but don't have to match any `T2`s or `T1`s, etc.

There are several type annotations with special meanings:

- `...` after a type name (included bracketed types) indicates variadic arguments or return values; one or more arguments of the given type are allowed, or one of more values of the given return type are returned.
- A lack of parentheses indicates a specification for a variable rather than a function, callback, message or method.
- A function or callback argument surrounded by square brackets (`[   ]`) has the same meaning as `Maybe<...>`.
- An entire invoked function name surrounded by square brackets means that the expected function can be an unset `nil` value without raising an error.
- Variadic arguments surrounded by square brackets can accept zero or more arguments.
- A comma-separated list of return types means the obvious — the callback or function returns a fixed number of multiple values (unless one or more specified return values are variadic).

> **Hardcoded message handlers:** A function or callback name surrounded by double quotes (`"   "`) and followed by a list of entity or context types in square brackets (`[   ]`) means the specified engine invocation or callback is actually an entity or world message whose invocation or reception code is hardcoded.
>
> _Example:_ `void` `"addCollectable"` [player] (`String` collection, `String` collectable)

> **Methods:** A function or callback name preceded by a type name immediately followed by a colon is actually a method available for userdata objects of that type. Userdata methods always take the object as their first arguments, hence the Lua colon syntax.
>
> _Example:_ `bool` `RpcPromise`:fulfilled()

## About Pluto

For documentation regarding Pluto, see the [Pluto documentation site](https://pluto-lang.org/docs/Introduction).

Note that compatibility mode, where all the new Pluto operators are prefixed with `pluto_`, is enabled by default — for script compatibility reasons, obviously!

## Deviations from standard Lua 5.4 / Pluto

The following base Lua callbacks and global variables differ from the standard Lua 5.4 / Pluto equivalents on xStarbound and stock Starbound:

#### Pluto libraries

Since `require` is modified, all Pluto libraries except `exception` are preloaded for you. Library table names:

- `base32` - [Base32 library](https://pluto-lang.org/docs/Runtime%20Environment/Base32).
- `base64` - [Base64 library](https://pluto-lang.org/docs/Runtime%20Environment/Base64).
- `bigint` - [BigInt library](https://pluto-lang.org/docs/Runtime%20Environment/Bigint).
- `cat` - [CaT library](https://pluto-lang.org/docs/Runtime%20Environment/CaT).
- `crypto` - [Cryptography library](https://pluto-lang.org/docs/Runtime%20Environment/Crypto).
- `http` - [HTTP library](https://pluto-lang.org/docs/Runtime%20Environment/HTTP). Not available with `"safeScripts"` for obvious reasons.
- `json` - [JSON library](https://pluto-lang.org/docs/Runtime%20Environment/JSON). `json.null` is recognised by Starbound's engine as a JSON `null` in all cases.
- `pluto_assert` - [Assertion library](https://pluto-lang.org/docs/Runtime%20Environment/Assert).
- `scheduler` - [Scheduler library](https://pluto-lang.org/docs/Runtime%20Environment/Scheduler).
- `socket` - [Socket library](https://pluto-lang.org/docs/Runtime%20Environment/Socket) Not available with `"safeScripts"` for obvious reasons.
- `url` - [URL library](https://pluto-lang.org/docs/Runtime%20Environment/Socket). Not available with `"safeScripts"` for obvious reasons.
- `vector3` - [Vector3 library](https://pluto-lang.org/docs/Runtime%20Environment/Vector3). Three-element vectors aren't particularly useful in Starbound though. For two-element vector manipulation, use `require "/scripts/util/vec2.lua"`.
- `xml` - [XML library](https://pluto-lang.org/docs/Runtime%20Environment/XML).

Note that Pluto's assertion library is loaded as `pluto_assert` instead of `assert` to avoid overwriting the standard Lua `assert` function and causing incompatibilities there.

#### `LuaValue` exception

You must invoke `require "/scripts/exception.lua"` to get access to Pluto's `exception`. Otherwise it's `nil`.

#### `Maybe<LuaTable>` \_SBLOADED

If `require` is used, this global table is populated with a map of loaded module scripts to `true` values. Example contents:

```lua
_SBLOADED = {
  ["/path/to/some/script/asset.lua"] = true,
  ["/path/to/another/script/asset.lua"] = true,
}
```

#### `JsonNull` null

The value of this global variable is recognised as a JSON `null` when passed to any xStarbound callback that accepts JSON. Equivalent to Pluto's `json.null`.

#### `bool, Variant<T..., LuaValue..., String>` pcall(`Variant<LuaFunction<T...(Maybe<LuaValue...>)>, LuaThread>` functionOrThreadVar, `LuaValue...` args)

The only difference from the standard `pcall` is that the Starbound version returns a more useful traceback by default when an error occurs.

#### `Maybe<Variant<String, LuaLightUserdata>>` require(`RawAssetPath` scriptPath)

As in standard Lua, this callback executes the specified script (as a `RawAssetPath`) at the site where it's called, populating the environment with that script's global variables. Unlike standard Lua, Starbound's `require` takes a raw asset path (see `RawAssetPath` in `root.md` for more details) and thus loads a raw asset, adding that asset's path to the global `_SBLOADED` table as a key (see above).

Additionally, Starbound's `require` does _not_ return the newly initialised script's environment table as a return value, but instead returns `nil` if no error occurs, a light userdata value if the loaded script hits the instruction or recursion limit, or an error string if any other error occurs.

The above means that, among other things, third-party libraries which expect to be loaded via a return value from `require` will need some minor modifications to be properly usable in Starbound Lua.

If `require` is used in a preprocessor script, it can only load scripts that were pre-loaded as assets prior to the execution of the `require`. `require` _can_ be used to load a script that was just created with `assets.add`.

> **Note:** On OpenStarbound, `require` is _not_ available in patch or preprocessor scripts. I.e., if `assets` is available (see `assets.md`), `require` is a `nil` value, not a callback.

#### `bool, Variant<String, LuaValue...>` coroutine.resume(`LuaThread` coroutine, [`LuaValue...` args])

The only difference from the standard `coroutine.resume` is that the Starbound version logs a more useful traceback by default when an error occurs.

## `"safeScripts"`

> **NOTICE: Support for unsafe legacy shared Lua states on xStarbound with `"safeScripts"` disabled has been deprecated in xStarbound v3.5. See _Function calls and data transfer across Lua/Pluto contexts_ below for more.**

`"safeScripts"` in `xclient.config` disables certain base Lua callbacks and libraries that allow potentially unsafe OS/filesystem access when `true` (the default), limiting the damage that malicious mods can do to your system.

The following variables, callbacks and libraries in Lua 5.4 and Pluto are disabled when `"safeScripts"` is enabled:

- `_G`
- `collectgarbage`
- `dofile`
- `load`
- `loadfile`
- `debug` library
- `io` library
- `os` library, _except_ the following time-related callbacks:
  - `os.clock`
  - `os.difftime`
  - `os.time`
  - `os.nanos`
  - `os.micros`
  - `os.millis`
  - `os.seconds`
  - `os.unixseconds`
- `package` library
- `http` library
- `socket` library

Additionally, see `require` above, since it's not available in some scripts regardless of whether `"safeScripts"` is enabled.

Note that Pluto's `dumpvar` and `exportvar` are always available on xStarbound regardless of `"safeScripts"` status. Consider using `dumpvar` or `exportvar` instead of `sb.print` for script debugging.

### Debugging with `"safeScripts"` enabled

Since the `debug` library is considered unsafe, if you need a traceback while `"safeScripts"` is enabled, consider using `pcall` (which is improved in Starbound Lua) and logging the second result if it fails. Also note that if an uncaught execution error happens, it will be logged anyway.

## Global callbacks

The following callbacks are available in all xStarbound Lua contexts:

#### `JsonArray` jarray([`LuaTable` array])

If an empty table or nothing is specified, creates an empty table set up to be recognised by the engine as a JSON array. If an existing, non-empty table is specified, the table is modified in place to be recognised by the engine as a JSON array, if possible (i.e., if the table contains only integer keys).

**Note:** In stock Starbound and StarExtensions, `jarray` cannot take any arguments.

#### `JsonObject` jobject([`LuaTable` object])

If an empty table or nothing is specified, creates an empty table set up to be recognised by the engine as a JSON object. If an existing, non-empty table is specified, the table is modified in place to be recognised by the engine as a JSON object, if possible (i.e., if the table contains only string or number keys, which are converted to strings using `fmt`).

**Note:** In stock Starbound and StarExtensions, `jarray` cannot take any arguments.

#### `size_t` jsize(`Variant<LuaTable, JsonArray, JsonObject>` arrayOrObject)

Gets the number of elements in a given JSON array or keys in a JSON object, including explicit `null`s. If given a `LuaTable` that isn't encoded as JSON via `jarray` or `jobject` (or as part of a callback return), `jsize` will determine whether it's looking at an array or a table by checking for keys that aren't unsigned integers.

#### `void` jremove(`Variant<LuaTable, JsonArray, JsonObject>` arrayOrObject, `LuaValue` key)

Removes an element or key from a JSON array or object in place, even if it's an explicit `null`.

#### `void` jresize(`Variant<LuaTable, JsonArray, JsonObject>` arrayOrObject, `size_t` targetSize)

Resizes a Lua table (usually a JSON array) so that it contains a given number of unsigned integer keys. If the Lua table contains any unsigned integer keys larger than the specified size, those keys are removed (and not set to JSON `null`s). On the other hand, if the largest unsigned integer key in the table is smaller than the specified size, additional `null` keys are added to make it up. Note that `jresize` will turn any unassigned `nil` "gaps" before the largest unsigned integer key into explicit `null`s.

### Assigning and removing `null` values in `JsonObject` and `JsonArray` tables

To assign a JSON `null` value on xStarbound, simply assign `null` or `json.null` instead of `nil`. However, if you want to ensure your mod is compatible with stock Starbound, StarExtensions, OpenStarbound, etc., read on.

- To _assign_ a `null` value to a nonexistent key in an encoded JSON array or object, simply assign `nil` _once_ to the new key _after_ setting or creating the table (i.e., not within braces). If the key _is_ already assigned to a non-`nil` value, assign `nil` to it _twice_. Just to be safe, you can always double-assign any key you want to be explicitly `null` — it won't unassign the `nil` by accident.
- To _remove_ a key with an explicit `null`, either use `jremove` or assign a non-`nil` value to the key and then reassign `nil` to it.

> **Note:** When creating a JSON array, you can simply add `nil`s on table creation as long as the last element you want to add in the array is _not_ `nil`. Any `nil` elements after the last `non`-nil element will be truncated.

> **Note:** On stock Starbound _et al._, `null` has a value of `nil` and works exactly like `nil`, while `json.null` results in a nil indexing error. As such, if you use xStarbound's `null`, you _can_ use it "compatibly", while `json.null` can't be used this way.

Example code:

```lua
local object = jobject{
  thisKey = "Some string.",
  thatKey = null, -- Assigns `null` to `"thatKey"` on xStarbound. Doesn't assign anything on stock Starbound and the like.
  otherKey = true
}

object.thisKey = null -- Assigns `null` to `"thisKey"` on xStarbound, but removes this key otherwise.

object.someKey = nil -- Assigns `null` to `"someKey"`.
object.otherKey = nil -- Removes `"otherKey"`

object.otherKey = nil -- Assigns `null` to `"otherKey"`.

object.someOtherKey = nil -- Assigns `null` to `"someOtherKey"`.
object.someOtherKey = 5 -- Assigns `5` to `"someOtherKey"`.
object.someOtherKey = nil -- Removes `"someOtherKey"`.

jremove(object, "someKey") -- Removes `"someKey"`

local array = jarray{nil, nil, true, nil} -- Array is actually `[null, null, true]`, with the fourth element truncated.

array[4] = nil -- Assigns `nil` to the fourth element. Array is now `[null, null, true, null]`.
```

## Function calls and data transfer across Lua/Pluto contexts

> **IMPORTANT NOTICE FOR SCRIPT MODDERS: Legacy support for shared Lua/Pluto states, and therefore unsafe cross-context Lua/Pluto state sharing and calls via sandbox exploits (i.e., smuggling values and calls through type metatables or base library tables), have been _deprecated and completely removed_ in xStarbound v3.5 — disabling `"safeScripts"` _no longer_ enables this functionality. Mods that use sandbox exploits to share state or make cross-context calls are _NO LONGER SUPPORTED_ by xStarbound.**

> **WARNING:** On stock Starbound, StarExtensions, OpenStarbound and the like, **be careful about sharing references to engine callbacks or methods via `shared` or any metatable/table hacks, as calling them _WILL_ cause segfaults («access violations» on Windows)!** When in doubt, use message handlers.
>
> Lua doesn't keep track of user-created references to callbacks after those callbacks get their lease "revoked" by the engine, which happens right after `uninit` finishes running in the context where the callback is leased, or for those "one-off" scripts without `uninit`, right after the script finishes running.

There are four main ways to share data and make calls across Lua contexts on xStarbound:

- `world.sendEntityMessage`, `universe.sendWorldMessage` and appropriate message handlers allow JSON values to be shared and function/callback calls to be made across contexts and even across world states. In addition to all the script contexts that can have message handlers in stock Starbound, OpenStarbound, etc., xStarbound supports setting message handlers in pane and universe client scripts as of v3.4.5.1+.
- `world.getGlobal`, `world.getGlobals`, `world.setGlobal` and `world.setGlobals` allow JSON values to be shared across contexts that have access to `world`. Only available on xStarbound.
  - Client-side, there is only a single world globals table that only gets cleared upon disconnection, exiting the client or returning to the main menu.
  - Server-side, each world has its own globals table that gets cleared when the world is unloaded.
- The `shared` table is available in all Lua contexts for sharing Lua values across contexts in the same Lua state. _On non-xStarbound clients and servers, this is potentially unsafe — calling «overleased» shared callback functions, or code containing calls to them, will cause segfaults!_
- JSON-serialisable values — or on non-xStarbound servers and clients, Lua/Pluto values — can be shared across contexts, and function/callback calls can be made across contexts, with `world.callScriptedEntity`, `world.callScriptContext`, `activeItem.callOtherHandScript` and the `callScriptArgs` argument to `world.entityQuery`. JSON sanitisation is enforced on all values shared this way on xStarbound. _On non-xStarbound clients and servers regardless, this is potentially unsafe, like `shared`!_

Two other ways of sharing data and making calls across Lua contexts exist on non-xStarbound clients and servers:

- Values stored in Lua/Pluto base library tables (`math`, `os`, `table`, `json`, etc.) are shared across contexts, just as for `shared`. (The metatables of these tables are inaccessible though.) _This has very limited utility on xStarbound due to enforced sandbox isolation. On non-xStarbound clients and servers, this is unsafe, like `shared`!_
- The "type metatables" shared across all values of each Lua type, other than tables and "full" userdata objects (not light userdata), are shared across contexts. _This has very limited utility on xStarbound due to enforced sandbox isolation. On non-xStarbound clients and servers, this is unsafe, like `shared`!_

Lua values cannot be shared across Lua states. On xStarbound, _all_ Lua contexts are run in fully isolated Lua/Pluto states, with the following safe exceptions:

- Universe client script contexts, which still share their state with each other.
- World server script contexts, which still share their state with each other on a per-world basis.

These are safe because the Lua states share the same callback set across all scripts and never run after `world` and interface-related callbacks are revoked.

However, on stock Starbound, OpenStarbound, _et al._, Lua states are only weakly separated as follows:

- Most client-side scripts for players, monsters, etc., share the same state with each other. The exceptions are noted below.
- Client-side universe scripts share their state with each other, but _not_ with other client-side scripts.
- All server-side scripts running on a given world share the same state for that world, but not with any other worlds. This is due to worlds being multithreaded on the server. For communication between worlds, use `message.setHandler` and `universe.sendWorldMessage` in a world server script (see `message.md` and `universeserver.md`).
- All server command processor scripts share the same state with each other, but not with any worlds. Again, it's due to multithreading. For communication with worlds, use `message.setHandler` and `universe.sendWorldMessage` (see `message.md` and `universeserver.md`).
- Augment, item creation and versioning scripts running on the same client or server share their state with each other, but not with any other scripts.
- Asset preprocessor and patch scripts share their state with all other such scripts on the same asset load or reload, whether on the server or client; state is _not_ shared across asset reloads.
- Client-side statistics scripts share their state with each other, but not with any other client-side scripts.
- The **Mod Binds** and **Voice Options** dialogues have their own states which they _don't_ share with each other.

> **Note:** World globals are shared across _all_ loaded primary and secondary players (and across player swaps!) on xStarbound, so be careful when using world globals to store global player state. Consider adding UUID checks (via `player.uniqueId`) if you need to "isolate" such global states — UUIDs are guaranteed to be unique to each loaded player as long as you use `player.uniqueId` or `entity.uniqueId` (but _not_ `world.entityUniqueId`, which can return a non-unique vanity UUID!).

## Common Lua functions called by the engine

The following is a list of common Lua functions called by the engine when running scripts, with their expected function signatures. (See `assets.md`, `versioning.md` and `statistics.md` for certain special cases.)

> **Note:** For scripting purposes, swapping to an unloaded player with `/swap` or `/swapuuid` counts as removing the old player and adding the new one.

- **`void` init():** Called upon initialisation in all script contexts except versioning scripts, asset preprocessor scripts and asset patch scripts.
  - For entity, status controller, generic player, companion and deployment scripts, initialisation happens when the entity is spawned in the world. Players are spawned in a world any time they warp to a new world, get added, or get revived.
  - For active and fireable items, initialisation happens when the item is initially held in an NPC or player's hand.
  - For status effects, initialisation happens when the status effect is added to the entity, and also when the entity is initialised while the effect is active.
  - For techs, initialisation happens when the tech is first added, and also when the player is initialised while the tech is active.
  - For local animator scripts, initialisation happens when the associated entity, tech or item is loaded on the client. For client-controlled players, unloading happens every time a player warps to a new world, gets added, or gets revived.
  - For scripted panes, initialisation happens when the pane is first opened, _before_ `displayed` (see below) is invoked.
  - See `statistics.md` for statistics scripts.
- **`void` uninit():** Called upon uninitialisation in all script contexts except versioning scripts, asset preprocessor scripts and asset patch scripts.
  - For entity, status controller, generic player, companion and deployment scripts, uninitialisation happens when the entity is despawned in the world. Players are despawned in a world any time they warp away from the world, get removed or die.
  - For active and fireable items, uninitialisation happens when the item stops being held in the NPC or player's hand.
  - For status effects, uninitialisation happens when the status effect expires or is removed from the entity, and also when the entity is uninitialised while the effect is active.
  - For techs, uninitialisation happens when the tech is removed, and also when the player is uninitialised while the tech is active.
  - For local animator scripts, uninitialisation happens when the associated entity, tech or item is unloaded on the client. For client-controlled players, unloading happens any time a player dies, is removed, or warps away from a world.
  - For scripted panes, uninitialisation happens when the pane is closed, _after_ `dismissed` (see below) is invoked.
  - See `statistics.md` for statistics scripts.
- **`void` update(`Args...` args):** Called once every `n` game ticks, where `n` is the update delta (1 by default), while the entity is running (for entity, status controller and all types of player scripts), the item is held (for item scripts), the tech or status effect is running (for techs and status effects), the pane is open (for panes), or the entity is loaded on the client (for local animator scripts). If `n` is or becomes zero, `update` will never get called. See `updatablescript.md` for more on update deltas and how they're changed. The engine passes different arguments to `update` across script contexts:
  - _`void` update(`float` dt):_ All updatable script contexts except those below. `dt` is the delta time in seconds that has passed since `update` was last called for the context.
  - _`void` update(`LuaTable` args):_ Tech scripts. `args` looks like this:
    ```lua
    {
      dt = 0.016667, -- Delta time; same as above.
      -- `moves` is a table showing whether given player input actions are currently active.
      -- In addition to normal input, xClient's player control callbacks will affect these.
      moves = { run = true, up = false, down = false, left = true, right = false,
                jump = false, primaryFire = false, altFire = false,
                special1 = false, special2 = false, special3 = false }
    }
    ```
  - _`void` update(`float` dt, `String` fireMode, `bool` shifting, `LuaTable` moveMap):_ Active item scripts. `fireMode` is one of `"primary"`, `"alt"` or `"none"`, and `shifting` is whether the player is holding the **Walk** key (<kbd>Shift</kbd> by default). `moveMap`, with all possible moves active, looks like this:
    ```lua
    {
      up = true,
      down = true,
      left = true,
      right = true,
      jump = true
    }
    ```
    Moves which are not active will be `nil`. In additional to normal game input, xClient's player control callbacks affect these as well.
  - _`void` update(`float` dt, `String` fireMode, `bool` shifting):_ Fireable item scripts. Like active item scripts, but without the `moveMap`.
- **`Maybe<Json>` onInteraction(`Json` interactionInfo):** Called when an NPC, monster, object or vehicle is interacted with. Expects a valid interact action config, `null` or `nil` (for no action) to be returned; on vehicles, a `nil` return lets a player or NPC lounge in the vehicle. For monsters and NPCs, `interactInfo` looks like this:

  ```lua
  jobject{
    sourceId = -4464, -- Entity ID of the interacting entity.
    sourcePosition, -- World position of the interacting entity.
  }
  ```

  For objects, `interactionInfo` looks like this:

  ```lua
  jobject{
    -- World position of the interacting entity relative to the object.
    source = jarray{3.338, -4.554},
    sourceId = -4464 -- Entity ID of the interacting entity.
  }
  ```

  For vehicles, `interactionInfo` looks like this:

  ```lua
  jobject{
    sourceId = -4464, -- Entity ID of the interacting entity.
    sourcePosition = jarray{657.54, 721.43}, -- World position of the interacting entity.
    interactPosition = jarray{660.12, 724.54} -- The entity's aim position when interacting.
  }
  ```

  _Note:_ For containers on xServer, a `nil` result returns the default container pane, whereas `null` returns _no_ pane.

- **`void` activate(`float` dt, `String` fireMode, `bool` shifting, `LuaTable` moveMap):** Called when an active item is being activated with the appropriate fire button or player control callback. Is passed the same arguments as `update` for active items. Other active item functions are self-explanatory.
- **`void` [click event callback] (`Vec2I` clickPosition, `int` button, `bool` buttonDown, `MouseButton` buttonName):** Called when a mouse button is clicked or released within a canvas widget that has an assigned callback name in the pane config's `"canvasClickCallbacks"`.

  In `"canvasClickCallbacks"`, each key represents the name of a canvas widget, and the corresponding value is the name of the callback function that the engine should invoke, replacing `[click event callback]`.

  If `buttonDown` is `true`, the mouse button is pressed down; otherwise, it is released. The `position` is relative to the lower-left corner of the canvas widget. `button` is the integer ID of the button, while `buttonName` is the name of the button. See `interface.md` for valid `MouseButton` values; the integer ID is the ordinal position of the string value in that list, minus 1.

- **`void` [key event callback] (`int` key, `bool` keyDown, `Key` keyName):** Called when a key is pressed or released within a canvas widget that has an assigned callback name in the pane config's `"canvasKeyCallbacks"`.

  In `"canvasKeyCallbacks"`, each key represents the name of a canvas widget, and the corresponding value is the name of the callback function that the engine should invoke, replacing `[key event callback]`.

  If `keyDown` is `true`, the key is pressed down; otherwise, it is released. `key` is the integer ID of the key, while `keyName` is the name of the key. See `interface.md` for valid `Key` values; the integer ID is the ordinal position of the string value in that list, minus 1.

- **`Json` shiftItemFromInventory(`Json` inputItemDescriptor):** Sequentially invoked on all pane scripts whenever an item in the player's inventory is **<kbd>Shift</kbd> + Left Click**'ed. If this function isn't defined or returns nothing, `nil` or `false`, the engine does nothing on this invocation. If this function returns `true`, the engine empties out the shift-clicked inventory slot on this invocation. If this function returns a valid item descriptor, the engine replaces the item slot with the returned item on this invocation. Every sequential invocation acts on the slot after it has been modified by previous invocations.

Other Lua functions invoked by the engine are fairly self-explanatory; see the base game assets for examples, and if that isn't enough, see xStarbound's source code for the gory details.
