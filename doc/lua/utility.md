# `sb` and `xsb`

The `sb` table contains miscellaneous utility functions that don't directly relate to any assets or content of the game, while the `xsb` table contains a single callback for returning the running xStarbound version. Available in all script contexts.

---

#### `String` xsb.version()

> **This callback and the `xsb` table are available only on xStarbound, for obvious reasons.**

Returns the currently running version of xStarbound. Returns xServer's version if running on xServer or xClient's version if running on xClient.

---

#### `double` sb.nrand([`double` standardDeviation], [`double` mean])

Returns a randomized value with a normal distribution using the specified standard deviation (default is `1.0`) and mean (default is `0.0`).

---

#### `String` sb.makeUuid()

Returns a `String` representation of a new, randomly-created `Uuid`.

---

#### `void` sb.logInfo(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Info log level. Logs to `xserver.log` if running on xServer or `xclient.log` if running on xClient.

---

#### `void` sb.logWarn(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Warn log level. Logs to `xserver.log` if running on xServer or `xclient.log` if running on xClient.

---

#### `void` sb.logError(`String` formatString, [`LuaValue` formatValues ...])

Logs the specified formatted string, optionally using the formatted replacement values, to the log file and console with the Error log level. Logs to `xserver.log` if running on xServer or `xclient.log` if running on xClient.

---

#### `void` sb.setLogMap(`String` key, `String` formatString, [`LuaValue` formatValues ...])

Sets an entry in the debug log map (visible while in debug mode) using the specified format string and optional formatted replacement values.

---

#### `String` sb.printJson(`LuaValue` value, [`bool` pretty])

Returns a human-readable string representation of the specified JSON value. If pretty is `true`, objects and arrays will have whitespace added for readability. Throws an error if the given Lua value can't be encoded into JSON.

---

#### `Json` sb.parseJson(`String` jsonToParse)
#### `Json` sb.parseJsonFragment(`String` jsonToParse)
#### `Json` sb.jsonFromString(`String` jsonToParse)

Attempts to parse the given text as JSON, returning a "JSONised" Lua value (with an appropriate metatable) that faithfully represents the parsed JSON text to any callbacks that accept JSON. If anything that isn't valid JSON is passed to any of these callbacks, an error will be thrown.

`sb.parseJsonFragment` accept any valid JSON value as a valid top-level value, while `sb.parseJson` and `sb.jsonFromString` accept only JSON arrays and objects as valid top-level values (as per the JSON standard).

---

#### `String` sb.print(`LuaValue` value)

Returns a human-readable string representation of the specified `LuaValue`.

---

#### `Variant<Vec2F, double>` sb.interpolateSinEase(`double` offset, `Variant<Vec2F, double>` value1, `Variant<Vec2F, double>` value2)

Returns an interpolated `Vec2F` or `double` between the two specified values using a sin ease function.

---

#### `String` sb.replaceTags(`String` string, `Map<String, String>` tags)

Replaces all tags (e.g., `<tag>`) in the specified string with the specified tag replacement values. See `root.md` for more information on tags.

---

#### `Json` sb.jsonMerge(`Json` a, `Json` b)

Returns the result of merging the contents of `b` on top of `a`. Any `null` values in `b` are (recursively) ignored in favour of non-`null` values in `a`.

Although ordinary Lua tables may be passed as `a` and `b`, tables should be created or converted with `jarray` (for arrays) or `jobject` (for objects) before being passed to this callback for best results.

---

#### `Json` sb.jsonQuery(`Json` content, `String` path, `Json` default)

Attempts to extract the value in the specified content at the specified path, and returns the found value or the specified default if no such value exists. An explicit `null` returns a `nil` instead of any non-`nil` default.

Although an ordinary Lua table may be passed as `content`, a table should be created or converted with `jarray` (for arrays) or `jobject` (for objects) before being passed to this callback for best results.

---

#### `int` sb.staticRandomI32([`LuaValue` hashValues ...])

Returns a statically randomized 32-bit signed integer based on the given list of seed values.

---

#### `int` sb.staticRandomI32Range(`int` min, `int` max, [`LuaValue` hashValues ...])

Returns a statically randomized 32-bit signed integer within the specified range based on the given list of seed values.

---

#### `double` sb.staticRandomDouble([`LuaValue` hashValues ...])

Returns a statically randomized `double` based on the given list of seed values.

---

#### `double` sb.staticRandomDoubleRange(`double` min, `double` max, [`LuaValue` hashValues ...])

Returns a statically randomized `double` within the specified range based on the given list of seed values.

---

#### `RandomSource` sb.makeRandomSource([`unsigned` seed])

Creates and returns a `RandomSource` userdata object which can be used as a random source, initialized with the specified seed. The `RandomSource` has the following methods:

---

##### `void` `[RandomSource]`:init([`unsigned` seed])

Reinitializes the random source, optionally using the specified seed.

##### `void` `[RandomSource]`:addEntropy([`unsigned` seed])

Adds entropy to the random source, optionally using the specified seed.

##### `uint32_t` `[RandomSource]`:randu32()

Returns a random 32-bit unsigned integer value.

##### `uint64_t` `[RandomSource]`:randu64()

Returns a random 64-bit unsigned integer value.

##### `int32_t` `[RandomSource]`:randi32()

Returns a random 32-bit signed integer value.

##### `int64_t` `[RandomSource]`:randi64()

Returns a random 64-bit signed integer value.

##### `float` `[RandomSource]`:randf([`float` min], [`float` max])

Returns a random `float` value within the specified range, or between `0.0` and `1.0` if no range is specified.

##### `double` `[RandomSource]`:randd([`double` min], [`double` max])

Returns a random `double` value within the specified range, or between `0.0` and `1.0` if no range is specified.

##### `int64_t` `[RandomSource]`:randInt([`int64_t` minOrMax], [`int64_t` max])

Returns a random signed integer value between `minOrMax` and `max`, or between `0` and `minOrMax` if no `max` is specified.

##### `uint64_t` `[RandomSource]`:randUInt([`uint64_t` minOrMax], [`int64_t` max])

Returns a random unsigned integer value between `minOrMax` and `max`, or between `0` and `minOrMax` if no `max` is specified.

##### `bool` `[RandomSource]`:randb()

Returns a random `bool` value.

---

**Note:** All integers in Lua are `int64_t`s by default, while all other numbers in Lua are `double`s by default. Note that Lua does not explicitly distinguish between integers and doubles, but converts numbers between the two types transparently.

`float`s, `int32_t`s and `uint32_t`s are lossly converted to the standard Lua number types, while `uint64_t`s may overflow into negative territory on implicit conversion.

---

#### `PerlinSource` sb.makePerlinSource(`Json` config)

Creates and returns a `PerlinNoise` userdata object which can be used as a Perlin noise source. The configuration for the `PerlinSource` should be a JSON object (consider using `jobject`) and can include the following keys:

- `uint64_t` __seed__ - Seed value used to initialize the source.
- `String` __type__ - Type of noise to use. Valid types are `"perlin"`, `"billow"` or `"ridgedMulti"`.
- `int64_t` __octaves__ - Number of octaves of noise to use. Defaults to `1`.
- `double` __frequency__ - Defaults to `1.0`.
- `double` __amplitude__ - Defaults to `1.0`.
- `double` __bias__ - Defaults to `0.0`.
- `double` __alpha__ - Defaults to `2.0`.
- `double` __beta__ - Defaults to `2.0`.
- `double` __offset__ - Defaults to `1.0`.
- `double` __gain__ - Defaults to `2.0`.

The `PerlinSource` has only one method:

---

##### `float` `[PerlinSource]`:get(`float` x, [`float` y], [`float` z])

Returns a `float` value from the Perlin source using 1, 2, or 3 dimensions of input.
