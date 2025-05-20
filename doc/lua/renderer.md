# `renderer`

> **These callbacks are only available on xStarbound v3.6.**

The `renderer` table contains bindings that pertain to the client's world renderer and is available in the following contexts:

- universe client scripts
- generic player scripts
- player companion scripts
- player deployment scripts
- tech scripts
- client-side NPC scripts
- client-side monster scripts
- client-side status effect and controller scripts
- client-side vehicle scripts
- client-side projectile scripts
- client-side active and fireable item scripts
- active item animation scripts
- monster and object animation scripts

---

#### `JsonObject` renderer.enabledLayers()

Returns the current status of all world rendering layers. The return value is as follows (showing defaults):

```lua
jobject{
  stars = true, -- The background stars.
  debrisFields = true, -- Includes asteroids rendered in the background. On non-shipworlds, these are specified in the world's metadata.
  backOrbiters = true, -- Includes any sun, planets or moons rendered in the background, but *not* the orbited planet or other object visible in the background on player ships. On non-shipworlds, these are specified in the world's metadata.
  planetHorizon = true, -- Includes the orbited planet or other object visible in the background on player ships.
  sky = true, -- Includes the sky colouring. Disabling this turns the sky black, but won't make the stars visible during daytime on a world.
  frontOrbiters = true, -- Includes clouds visible over an orbited planet.
  parallax = true, -- Includes the world's parallax layers as specified in its metadata's parallax config. Disabling this layer can significantly improve the client's framerate.
  entities = true, -- Includes all entities. This includes objects and plants, as well as local animator drawables. Placed here in order because this is the lowest layer at which entities *can* be rendered.
  backgroundOverlays = true, -- Includes ship sprites rendered behind background tiles. Yes, these aren't actually objects. They're specified in the world's metadata under `"centralStructure"` → `"backgroundOverlays"`.
  backgroundTiles = true, -- Background tiles.
  midgroundTiles = true, -- Miground tiles. Platforms are considered "midground".
  particles = true, -- Particles. Disabling this layer can improve the client's framerate.
  liquids = true, -- Liquids.
  foregroundTiles = true, -- Foreground tiles.
  overlays = true, -- Various foreground overlays. Does not include local animator drawables; those are considered part of the entity layer.
  nametags = true, -- Name tags rendered over entities.
  bars = true, -- Bars rendered over entities. In vanilla Starbound, this is used only for rendering the shield health bar.
}
```

---

#### `void` renderer.setEnabledLayers(`Json` layerConfig)

Sets the rendering status of specified rendering layers. If a JSON object is passed, the object is expected to be in a format similar to that of `renderer.enabledLayers`, any missing keys or `null` values are left as is. For instance, `jobject{ parallax = false }` disables the parallax layer while leaving all other layers at their current status. If no argument is specified, or `nil` or `null` is passed, the default — where all layers are enabled — is restored.

---

#### `void` renderer.overrideFullbright(`bool` override)

Sets whether fullbright will be enabled regardless of the `/fullbright` command's status. Useful in scripts that alter client-side world rendering.

---

#### `bool` renderer.fullbrightOverridden()

Returns whether fullbright is enabled via the overriding callback above. Does _not_ return whether fullbright is enabled via the `/fullbright` command.

---

#### `JsonObject` renderer.tileDirectives()

Returns the current tile layer rendering directives. The return value is as follows (showing defaults):

```lua
jobject{
  liquids = "", -- Directives used when rendering liquids. Note that liquids are also tinted by their liquid colour after these directives.
  background = "", -- Directives used when rendering background tiles.
  midground = "", -- Directives used when rendering midground tiles (e.g., platforms).
  foreground = "", -- Directives used when rendering foreground tiles.
}
```

---

#### `void` renderer.setTileDirectives(`Json` tileDirectives)

Sets tile layer rendering directives. If a JSON object is passed, it is expected to be in a format similar to what `renderer.tileDirectives` returns, but any unspecified keys will leave that layer's directives unmodified. If the argument is unspecified, or `nil` or `null` is passed, all tile layer rendering directives are removed.

**Note:** To apply directives to _entities_ (including objects and plants), used `world.setEntityDirectives` (see `world.md`). To change the appearance of the background behind tiles, consider hiding all rendering layers below the background tiles with `renderer.setEnabledLayers` and adding local animator drawables to the `"BackgroundOverlay"` layer.
