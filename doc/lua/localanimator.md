# `localAnimator`

The `localAnimator` table provides bindings used by client-side animation scripts on monsters, objects, vehicles, active items and player deployment scripts to set drawables/lights and perform rendering actions. Also see the client-side `world.setShaderParameters`, `world.getShaderParameters` and `world.resetShaderParameters` bindings in `world.md`. Note that, unlike other contexts, drawables rendered by player deployment scripts only show up for the client controlling that player.

**Note:** All `Vec2F` positions are in world *tiles* (8 world pixels apiece), not world pixels or interface pixels.

---

#### `void` localAnimator.playAudio(`String` sound, [`int` loops], [`float` volume])

Immediately plays the specified sound, optionally with the specified loop count and volume.

---

#### `void` localAnimator.spawnParticle(`Json` particleConfig, `Vec2F` position)

Immediately spawns a particle with the specified name or configuration at the specified position.

---

#### `void` localAnimator.addDrawable(`Drawable` drawable, [`String` renderLayer], [`bool` ignoreScaling])

Adds the specified drawable to the animator's list of drawables to be rendered. If a render layer is specified, this drawable will be drawn on that layer instead of the parent entity's render layer. Unless `ignoreScaling` is `true`, the drawable is implicitly scaled by ×1/8 to keep it from appearing too large on screen; this implicit scaling may interfere with drawable positioning in certain ways. Drawables set in this way are retained between script ticks and must be cleared manually using `localAnimator.clearDrawables()`.

The drawable object must specify *one* of the following keys to define its type:

* [`pair<Vec2F, Vec2F>` __line__] - Defines this drawable as a line between the specified two points.
* [`List<Vec2F>` __poly__] - Defines the drawable as a polygon composed of the specified points.
* [`String` __image__] - Defines the drawable as an image with the specified asset path.

The following additional keys may be specified for any drawable type:

* [`Vec2F` __position__] - World position of the drawable. Otherwise defaults to `jarray{0.0, 0.0}`.
* [`Color` __color__] - Colour for the drawable. Defaults to `"white"` (i.e., `jarray{255, 255, 255, 255}`).
* [`bool` __fullbright__] - Specifies whether the drawable is fullbright (i.e., ignores world lighting).

The following additional key may be specified for line drawables:

* [`float` __width__] - Specifies the width of the line to be rendered.

The following transformation options may be specified for image drawables. Note that if a __transformation__ is specified, it will be executed before any other specified operations.

* [`Mat3F` __transformation__]
* [`bool` __centered__]
* [`float` __rotation__] - In radians.
* [`bool` __mirrored__] - X-axis mirroring.
* [`float` __scale__]

---

#### `void` localAnimator.clearDrawables()

Clears the list of drawables to be rendered.

---

#### `void` localAnimator.addLightSource(`Json` lightSource)

Adds the specified light source to the animator's list of light sources to be rendered. Light sources set in this way are retained between script ticks and must be cleared manually using `localAnimator.clearLightSources()`. The configuration object for the light source accepts the following keys:

* `Vec2F` __position__ - World position. Required.
* `Color` __color__ - Required.
* [`bool` __pointLight__] - If `true`, this is a point light source; otherwise it's a regular «splotch» light source.
* [`float` __pointBeam__]
* [`float` __beamAngle__] - In radians.
* [`float` __beamAmbience__]

---

#### `void` localAnimator.clearLightSources()

Clears the list of light sources to be rendered.
