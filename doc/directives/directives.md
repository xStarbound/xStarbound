# Image processing directives

Image processing directives are used to modify a base image asset before it is displayed by the game engine.

### Syntax

The directive syntax is as follows:

- _`?op`:_ This defines an image processing operation, where `op` is the name of the operation.
- _`=arg`:_ An argument to an image processing operation. Must follow either an `?op` specifier or another argument.
- _`;arg`:_ Same as above. Semicolons (`;`) and equal signs (`=`) are freely interchangeable.

> _Example:_ The directive string `?scale=0.4?scale=0.7?scale=0.84?crop;4;2;5;3` contains three scale operations with arguments `0.4`, `0.7`, and `0.84`, respectively, specifying scale multipliers, and one crop operation with arguments `4`, `2,` `5`, and `3`, specifying the cropping bounds in image pixels.

### Execution order

Image processing operations are executed in sequential order, with each operation acting on the image potentially modified by the last.

### Argument types

The following types of arguments are used in directive operations.

- `int`: A signed integer.
- `uint`: An unsigned integer.
- `float`: A floating-point value.
- `path`: A single asset path, of `AssetPath<Image>` type.
- `path+`: A list of asset paths, of `AssetPath<Image>` type, split by plus signs (`+`) if there is more than one specified asset.
- `colour`: A valid hex colour code. Can be in `RGB`, `RGBA`, `RRGGBB` or `RRGGBBAA` format.
- `skip`: The optional literal `skip`, only used in `scalenearest`.

**Image asset paths:** Question marks (`?`), semicolons (`;`) and equal signs (`=`) cannot be used in image asset names, or the names of directories containing them, because of their special meaning to the directive parser. For `path+` arguments, plus signs (`+`) are also disallowed. As such, for this and other reasons, asset file and directory names should be restricted to only alphanumeric characters.

**X and Y values:** As with the `Image` methods (see `$doc/lua/image.md`), all directive operations assume the image origin is at the _lower left_ corner of the image, with increasing Y values going _up_. This is opposite to how most image editors handle coordinates.

### Supported image operations

xStarbound supports the image operations listed below. Names and arguments are in `operation=arg1;arg2[;arg3][;arg4=arg5;arg4=arg5...]` syntax, where `operation` is the name of the operation, `arg1` and `arg2` are regular arguments, `arg3` is an optional argument, and `arg4=arg5...` is an optional arbitrarily long (or short) series of paired arguments.

- **`hueshift=degrees`** Shifts the hues of the image by `degrees` (`float`) in degrees, as if on a colour wheel.
- **`saturation=percentage`:** Increases or decreases the image's colour saturation by `percentage` (`float`). `-100` fully desaturates the image, making it black-and-white, while `100` fully blows out the colours.
- **`brightness=percentage`:** Increases or decreases the image's brightness by `percentage` (`float`). `-100` fully darkens the image while `100` fully blows out the brightness.
- **`fade=colour;amount`:** "Fades" the image toward a given `colour` (`colour`) by a given `amount` (`float`). An amount of `1.0` fully replaces the image with the specified colour.
- **`scanlines=colour1;amount1;colour2;amount2`:** `colour1` (`colour`), `amount1` (`float`), `colour2` (`colour`), `amount2` (`float`). Like `fade`, but applies an alternating "scanlines" effect, where "even" pixel lines use the one set of values and "odd" pixel lines use the other set. Used by the scanning tool.
- **`setcolor=colour`:** Sets all image pixels with an alpha above 0 to the specified `colour` (`colour`). Ignores the alpha value in the specified colour; it's always `ff`. Use a subsequent `?multiply=ffffffAA`, where `AA` is the desired alpha value, to add alpha transparency.
- **`replace[;source=replacement;source=replacement...]`:** Replaces each specified `source` colour (`colour`) with a corresponding `replacement` (`colour`). The list of argument pairs can be arbitrarily long, and often is _extremely_ long in generated sprite directives. xStarbound, OpenStarbound and StarExtensions have special optimisations for such extremely long sequences of `replace` arguments.
- **`addmask=masks;x;y`:** Does an additive mask operation on the base image using the provided list of images in `masks` (`path+`). The mask(s) is/are offset from the base's origin by the specified `x` (`int`) and `y` (`int`) values.
- **`submask=masks;x;y`:** `masks` (`path+`), `x` (`int`), `y` (`int`). Same as above, but subtractive.
- **`blendmult=blendImages;x;y`:** `blendimages` (`path+`), `x` (`int`), `y` (`int`). Multiplicatively blends the specified images into the base image, which may be offset by the specified offset.
- **`blendscreen=blendImages;x;y`:** `blendimages` (`path+`), `x` (`int`), `y` (`int`). Same as above, but does division instead of multiplication.
- **`multiply=multColour`:** Multiplies all pixel colour values in the image by the given `multColour` (`colour`).
- **`border=pixels;startColour;endColour`:** Adds a border that is a specified number of `pixels` (`uint`) thick, Starting with the `startColour` (`colour`) on the inside, the border colour transitions to the `endColour` (`colour`) on the outside. If the border is one pixel thick, the `startColour` and `endColour` will be mixed together in the one-pixel border, with no transition. The pixel width is capped to 128 on xStarbound to prevent potential exploits. For the purpose of calculating the distance from the nearest non-transparent pixel to find edges, an image pixel (prior to this directive's application) must have an alpha value of `0` / `0x0` (invisible) to count as «transparent»; otherwise it is opaque and no border is drawn over it.
  - _xStarbound/OpenStarbound text directives:_ On xStarbound and OpenStarbound, `border` directives in text escape codes count any pixel that is not fully opaque (i.e., one with an alpha value other than `255` / `0xff`) as «transparent» for edge finding, and additionally will blend the border colour into the original pixel colour, making it appear as if the border were underlaid.
- **`outline=pixels;startColour;endColour`:** `startColour` (`colour`), `endColour` (`colour`), `pixels` (`uint`). Same as above, but also makes the base image pixels invisible.
- **`scale=factor[;factorY]`:** Scales the image by the given scale `factor` (`float`) on both axes; if `factorY` is also specified, `factor` is the X scaling factor and `factorY` is the Y scaling factor. Uses bilinear scaling. Capped to `4096` to prevent exploits on xStarbound; additionally, negative values log a warning on xStarbound.

> **WARNING:** Using `?scale` with a negative value on a drawable will **crash** any stock Starbound client that renders that drawable. You deserve any server ban you get for doing this sort of malicious shit.

- **`scalebilinear=factor[;factorY]`:** Same as above.
- **`scalebicubic=factor[;factorY]`:** Same as above, but uses bicubic scaling instead.
- **`scalenearest=factor[;factorY][;skip]`:** Same as above, but uses nearest-pixel scaling. On xStarbound, OpenStarbound and StarExtensions, nicer nearest-_screen_-pixel scaling is used when this directive is in a player or NPC's tech, status controller or status effect parent directives. This nicer scaling can be disabled by optionally including `skip`; this argument can appear anywhere in the argument list on xStarbound, but should be placed at the end on other clients.
- **`crop=leftX;bottomY;rightX;topY`:** `leftX` (`int`), `bottomY` (`int`), `rightX` (`int`), `topY` (`int`). Crops the image to the specified coordinates. On xStarbound, out-of-bounds cropping is allowed, but only the in-bounds portion is returned.
- **`flipx`:** Flips the image on the X axis.
- **`flipy`:** Same, but on the Y axis.
- **`flipxy`:** Same, but on both axes.
- **`setpixel=x;y;newColour` [xStarbound only]:** Sets the pixel at the given `x` (`uint`) and `y` (`uint`) coordinates to the given `newColour` (`colour`).
- **`blendpixel=x;y;newColour` [xStarbound only]:** `x` (`uint`), `y` (`uint`), `newColour` (`colour`). Same as above, but uses alpha blending to blend the pixel into the base image if the alpha isn't `ff`.
- **`copyinto=image;x;y`:** Copies the pixels of the specified `image` (`image`) into the base image, _replacing_ in-bounds pixels of the base image with those of the copied image. The specified image to copy is offset from the base's origin by the specified `x` (`uint`) and `y` (`uint`) values.
- **`drawinto=image;x;y`:** `image` (`image`), `x` (`uint`), `y` (`uint`). Same as above, but alpha-blends the copied image's pixels into the base image. Preferable if you don't want the copied image's transparent pixels to «cut out» portions of the base.

### Technical note for macOS users

Generated sleeves on the x86 macOS build (at least on the latest x86-64 Apple Clang on macOS 14 Sonoma) are likely affected by an optimisation «fluke» (that affects four back sleeve frames) which I couldn't fix by disabling optimisations and didn't feel like patching around, so they're most likely still in that build if you decide to compile it yourself. I suggest you save yourself the trouble by cross-compiling and running the Windows build in Whisky or WINE.
