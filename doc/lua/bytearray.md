# `ByteArray` methods

> **`ByteArray` objects and their methods are available only on xStarbound.**

`ByteArray` userdata objects can be created or loaded with `root.newBytes`, `root.bytes`, `assets.newBytes` or `assets.rawBytes` (see `root.md` and `assets.md`).

`ByteArray`s are a useful way to sidestep the complexities and annoyances of UTF-8 when manipulating strings, and can also be manipulated to modify other types of asset files, such as audio files or raw PNGs.

> **Technical note:** If you pass any integers that are less than `0` or more than `255` to these methods, they will simply be wrapped around to stay within a range of `0` to `255`, rather than causing an error to be thrown. This allows you to specify `unsigned char` values, among other things.

These objects have the following methods available:

#### `ByteArray` `[ByteArray]`:size()

Returns a *copy* of this `ByteArray` object.

#### `size_t` `[ByteArray]`:size()

Returns the size of the byte array in bytes.

#### `String` `[ByteArray]`:get()

Returns the contents of the byte array as a UTF-8 string. Note that the returned string will terminate just before the first null byte in the byte array, if there are any.

#### `List<uint8_t>` `[ByteArray]`:getBytes()

Returns the contents of the byte array as an array of unsigned 8-bit integers.

#### `String` `[ByteArray]`:getByte(`size_t` pos)

Returns the given byte as a Lua string. Indexing is `1`-based for consistency with Lua. Will return an empty string, not `"\0"`, if the byte is null.

Will throw an error if an out-of-bounds index is used.

#### `uint8_t` `[ByteArray]`:getByteChar(`size_t` pos)

Returns the given byte as an 8-bit unsigned integer. Indexing is `1`-based for consistency with Lua.

Will throw an error if an out-of-bounds index is used.

### `ByteArray` `[ByteArray]`:set(`Variant<List<uint8_t>, ByteArray, String>` newBytes)

Sets the byte array to the given array of unsigned 8-bit integers, a copy of the given `ByteArray` object, or to the given Lua string of bytes. Returns the modified base `ByteArray`, which is modified in place.

Note that Lua supports byte escape codes in strings, and that byte arrays do not need to terminate with a null byte (`\0`).

### `ByteArray` `[ByteArray]`:setByte(`Variant<String, uint8_t>` newByte)

Sets the byte at the given position in the byte array to the specified byte in the form of an unsigned 8-bit integer or a Lua string. Indexing is `1`-based for consistency with Lua. Returns the modified `ByteArray`, which is modified in place.

Will throw an error if an out-of-bounds index is used or the string is not one byte long.

Note that Lua supports byte escape codes in strings, and that byte arrays do not need to terminate with a null byte (`\0`).

#### `ByteArray` `[ByteArray]`:clear()

Empties the byte array in place, setting it to a length of zero bytes. Returns the now-emptied byte array.

#### `ByteArray` `[ByteArray]`:append(`Maybe<Variant<uint8_t, List<uint8_t>, ByteArray, String>>` byteSequence, `Maybe<size_t>` times)

Sets the byte array to the given byte sequence, passed as one of a `ByteArray` object, Lua string or unsigned 8-bit integer (for a single potentially repeated byte). Returns the modified `ByteArray`, which is modified in place.

If the specified byte sequence is `nil` or unspecified, the desired byte sequence is assumed to be a null byte (`\0`).

The byte sequence will be appended to the byte array a number of times equal to `times`, or only once if `times` is `nil` or unspecified.

#### `ByteArray` `[ByteArray]`:getSubBytes(`size_t` position, `size_t` length)

Slices a sub-`ByteArray` out of this `ByteArray`, starting at the specified position and having the specified length. Indexing for the `position` is `1`-based for consistency with Lua. Does not modify the base `ByteArray`.

Will throw an error if the slice is out of bounds.

**Tip:** You can use `ba:getSubBytes(1, ba:size())` instead of `ba:copy()` to get a copy of a given byte array `ba`.