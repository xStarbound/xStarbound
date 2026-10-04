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
- `path`: A single asset path, of `AssetPath<Image>` type (which supports a frame specifier).
- `path+`: A list of asset paths, of `AssetPath<Image>` type (which supports a frame specifier), split by plus signs (`+`) if there is more than one specified asset.
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
- **`copyinto=image;x;y` [xStarbound only]:** Copies the pixels of the specified `image` (`path`) into the base image, _replacing_ in-bounds pixels of the base image with those of the copied image. The specified image to copy is offset from the base's origin by the specified `x` (`uint`) and `y` (`uint`) values.
- **`drawinto=image;x;y` [xStarbound only]:** `image` (`path`), `x` (`uint`), `y` (`uint`). Same as above, but alpha-blends the copied image's pixels into the base image. Preferable if you don't want the copied image's transparent pixels to «cut out» portions of the base.

### Technical note for macOS users

Generated sleeves on the x86 macOS build (at least on the latest x86-64 Apple Clang on macOS 14 Sonoma) are likely affected by an optimisation «fluke» (that affects four back sleeve frames) which I couldn't fix by disabling optimisations and didn't feel like patching around, so they're most likely still in that build if you decide to compile it yourself. I suggest you save yourself the trouble by cross-compiling and running the Windows build in Whisky or WINE.

# Text escape codes

Text escape codes are used to modify the appearance of text before it is displayed by the game engine.

### Syntax

The escape code syntax is as follows:

- `^`: A caret. Begins a text escape code sequence.
- `colour`: This defines a text colour code operation, where `colour` is the argument for this operation.
- _`op`:_ This defines a non-colour text processing operation, where `op` is the name of the operation.
- _`=arg`:_ An argument to a non-colour text processing operation. Must follow an `op` specifier.
- `,`: A comma. Separates text processing operations.
- `;`: A semicolon. Ends a text escape code sequence.

A valid text escape code sequence begins with a caret (`^`), contains zero or more comma-separated (`,`) operations (or arbitrary text strings) of either `colour`, `op` or `op=arg` format, and ends with a semicolon (`;`). Any unrecognised operation (i.e., any arbitrary text that doesn't match a valid operation) is silently ignored. Once processed, the text of the escape code sequence itself is visually hidden for all display purposes as if the text weren't even there (not even as invisible space).

If processing is inside an escape code sequence, a semicolon is always taken as the end of an escape code sequence, and if processing is not inside an escape code sequence, a caret is always taken as the beginning of an escape code sequence.

> _Example:_ The escape code sequence `^blue,font=hobo,shadow;` contains a colour operation with the argument `blue`, a `font` operation with the argument `hobo` and a `shadow` operation that takes no argument, all processed in that order.

**Lazy matching caveat:** The escape code processor looks for the _last_ caret before a given semicolon when looking for valid escape sequences (using lazy matching), so any other carets present before that semicolon (and after any previous semicolon, whether part of an escape code or not, or after the beginning of the text string if there are no prior semicolons) are displayed as literals and do _not_ start escape code sequences.

**Escaping carets:** To display a literal caret, use `^^;`. This uses the processor's lazy matching such that the displayed caret is the _first_ of the two carets, followed by an empty escape code sequence (`^;`). Replacing the initial `^` in an escape code sequence with `^^;` allows the escape code sequence to be literally displayed in chat instead of being hidden and applied to the following text, which can be handy.

### Supported escape operations

- **`<colour>`:** A `colour` operation. The argument is the specified `<colour>` that names the operation. Accepts two types of arguments:
  1. A valid hex colour code preceded by a `#`. Can be in `#RGB`, `#RGBA`, `#RRGGBB` or `#RRGGBBAA` format.
  2. A valid colour name. See _Colour names_ below for a list of valid names.

  Any invalid argument (i.e., arbitrary text that is neither a valid `#`-marked colour code nor a valid colour name) is simply quietly ignored and does nothing.

- **`reset`:** Takes no argument. Clears the effects of any prior escape code operations (even in the same sequence!), and resets the appearance of subsequent text to whatever was saved with the previous `set` operation (or the defaults, if there is no prior `set`). The base defaults are found under `"font"` in `$assets/interface.config`; see the relevant file under `$src/assets/xSBassets/interface.config.patch` for xStarbound-specific defaults. Note that the widget config for each text widget may specify its own default font colour that overrides the base defaults, and there are also separate colour defaults for the `/debug` interface and name tags that also override the base defaults if specified.
- **`set`:** Takes no argument. Stores the current escape code overrides as the «defaults» for any subsequent `reset` operation. Note that «current overrides» include the effects of any preceding operations (including `reset`s) in the same escape code sequence as the `set` command.
- **`shadow`:** Takes no arguments. Enables a visual shadow for subsequent displayed text, which can make white or brightly coloured text more easily readable against a white or bright background.
- **`noshadow`:** Takes no arguments. Disables the visual shadow for subsequent displayed text.
- **`font=<font>`:** Takes a `<font>` argument for the name of the font to use (e.g., `font=hobo`). Uses the specified font for subsequent displayed text until the font is changed or reset by another escape code. Font changes affect text wrapping appropriately. If the specified font name is invalid or does not refer to any font that exists in the assets, this resets to the configured default font for subsequent text.
- **`directives=<directives>`:** Takes a `<directives>` argument for the image processing directives to use. Applies the specified processing directives to subsequent displayed text glyphs until the directives are changed or reset by another escape code. Any invalid directives render the subsequent text invisible. See _Image processing directives_ above for valid directive syntax and operations. Note that since semicolons (`;`) are interpreted as an escape sequence terminator, they can't be used in the directives specified for this operation, but that's not a big deal since equal signs (`=`) are accepted in directives wherever semicolons are accepted. As another caveat, if you're using this for custom sprites (for, e.g., colourful emoji), each pixel is directly equivalent to a screen pixel, not scaled (even if the font size or interface scale is changed!).

### Colour names

The following named colours are understood by Starbound.

| Name             | Hex Code    | RGBA (Decimal)       |
| ---------------- | ----------- | -------------------- |
| `red`            | `#FF4942FF` | (255, 73, 66, 255)   |
| `orange`         | `#FFB42FFF` | (255, 180, 47, 255)  |
| `yellow`         | `#FFEF1EFF` | (255, 239, 30, 255)  |
| `green`          | `#4FE646FF` | (79, 230, 70, 255)   |
| `blue`           | `#2660FFFF` | (38, 96, 255, 255)   |
| `indigo`         | `#4B0082FF` | (75, 0, 130, 255)    |
| `violet`         | `#A077FFFF` | (160, 119, 255, 255) |
| `black`          | `#000000FF` | (0, 0, 0, 255)       |
| `white`          | `#FFFFFFFF` | (255, 255, 255, 255) |
| `magenta`        | `#DD5CF9FF` | (221, 92, 249, 255)  |
| `darkmagenta`    | `#8E2190FF` | (142, 33, 144, 255)  |
| `cyan`           | `#00DCE9FF` | (0, 220, 233, 255)   |
| `darkcyan`       | `#0089A5FF` | (0, 137, 165, 255)   |
| `cornflowerblue` | `#6495EDFF` | (100, 149, 237, 255) |
| `gray`           | `#A0A0A0FF` | (160, 160, 160, 255) |
| `lightgray`      | `#C0C0C0FF` | (192, 192, 192, 255) |
| `darkgray`       | `#808080FF` | (128, 128, 128, 255) |
| `darkgreen`      | `#008000FF` | (0, 128, 0, 255)     |
| `pink`           | `#FFA2BBFF` | (255, 162, 187, 255) |
| `clear`          | `#00000000` | (0, 0, 0, 0)         |

Colour names are handled case-insensitively. These names are accepted wherever a text colour can be specified.
