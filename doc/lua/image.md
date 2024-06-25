# `Image` methods

> **`Image` objects and their methods are available only on xStarbound and OpenStarbound.**

`Image` userdata objects can be created or loaded with `root.newImage`, `root.image`, `assets.newImage` or `assets.image` (see `root.md` and `assets.md`). These objects have the following methods available:

> **Note on image origins:** The image origin for these callbacks is the *lower* left corner, with increasing Y values going *up*. This is opposite to the way coordinates are handled in most image editors.

#### `Image` `[Image]`:copy()

> **Available only on xStarbound.**

Returns a *copy* of this `Image` object.

#### `Vec2U` `[Image]`:size()

Returns the pixel size of this `Image` object.

#### `Variant<Vec3B, Vec4B>` `[Image]`:get(`uint32_t` x, `uint32_t` y)

Gets the RGBA colour of the specified pixel as a `Vec3B` if the alpha value is `255` (fully opaque), or as a `Vec4B` otherwise.

#### `String` `[Image]`:getHex(`uint32_t` x, `uint32_t` y)

> **Available only on xStarbound.**

Gets the RGBA colour of the specified pixel as a hex string in `"RRGGBB"` format if the alpha value is `ff` (fully opaque), or in `"RRGGBB"` format otherwise.

#### `Image` `[Image]`:set(`uint32_t` x, `uint32_t` y, `Variant<Vec3B, Vec4B>` newColour)

> *Returns `void` on OpenStarbound.*

Sets the RGBA colour of the specified pixel. If the specified colour is a `Vec3B`, the alpha value will be set to `255`. Returns the modified `Image` *and* modifies it in place.

Equivalent to the `setpixel` image directive.

#### `Image` `[Image]`:setHex(`uint32_t` x, `uint32_t` y, `String` newHexColour)

> **Available only on xStarbound.**

Sets the RGBA colour of the specified pixel. Accepts all four possible hex formats — `"RRGGBBAA"`, `"RRGGBB"`, `"RGBA"` and `"RGB"`. Formats missing an alpha will set the alpha to `ff`. Formats with one character per colour will effectively double each character — e.g., `9a0f` becomes `99aa00ff`. Returns the modified `Image` *and* modifies it in place.

Equivalent to the `setpixel` image directive.

#### `Image` `[Image]`:subImage(`Vec2U` lowerLeft, `Vec2U` size)

> *Returns `void` on OpenStarbound.*

Returns a cropped portion of the base image with the specified lower-left corner position and size relative to that corner. The size is automatically clamped so as to not go beyond the base image's bounds, if applicable. The base image is *not* modified in place.

Roughly equivalent to the `crop` image directive.

**Tip:** You can use `im:subImage({0, 0}, im:size())` instead of `im:copy()` to get a copy of a given image `im`.

#### `Image` `[Image]`:process(`Directives` directives)

> *Returns `void` on OpenStarbound.*

Applies the specified image processing directives to the image. The `Image` is modified in place and also returned.

**Note:** In asset preprocessing scripts, processing any directives that reference another image (i.e., `blendmult`, `blendscreen`, `addmask` and `submask`) will throw an error due to the assets not being fully loaded yet. Tack those directives onto the image path specified in `assets.image` instead.

#### `Image` `[Image]`:copyInto(`Vec2U` lowerLeft, `Image` subImage)

> *Returns `void` on OpenStarbound.*

Copies the pixels of the specified image into the base image as a sub-image, using the specified lower-left corner position for sub-image placement. The base `Image` is modified in place and also returned.

With this callback, pixels in the base image are *replaced* by sub-image pixels.

The sub-image will be cropped off if it goes beyond the base image's bounds.

Equivalent to the `copyinto` image directive.

#### `Image` `[Image]`:drawInto(`Vec2U` lowerLeft, `Image` subImage)

> *Returns `void` on OpenStarbound.*

Draws the pixels of the specified image into the base image as a sub-image, using the specified lower-left corner position for sub-image placement. The base `Image` is modified in place and also returned.

With this callback, pixels in the sub-image are *blended* into the base image using alpha blending, like an overlay.

The sub-image will be cropped off if it goes beyond the base image's bounds.

Equivalent to the `drawinto` image directive.