# `celestial`

The `celestial` table contains functions that relate to the client sky, flying the player ship, system positions for planets, system objects, and the celestial database. It is available to scripts running in the following contexts on players:

- script panes
- quests
- generic player scripts
- universe client scripts

A variant of `celestial` containing only `celestial.parameters` is avaiilable to JSON versioning scripts (see below).

**Warning:** The celestial database will **NOT** be fully loaded when a celestial callback is first invoked in a given context! Some callbacks — noted below — return `nil` or a dummy value when the celestial database isn't loaded. If you use these callbacks in a script, make sure to put them behind a conditional check or coroutine in `update` that waits until `celestial.planetName(celestial.currentSystem())` stops returning `nil`.

#### `bool` celestial.skyFlying()

Returns whether the client sky is currently flying.
---

#### `String` celestial.skyFlyingType()

Returns the type of flying the client sky is currently performing. The following flying types exist:

- `"none"`: Not flying at all.
- `"disembarking"`: Flying away from a world orbit.
- `"warp"`: Warping through space. Use `celestial.skyWarpPhase` to get the current warp phase.
- `"arriving"`: Flying into a world orbit.
---

#### `String` celestial.skyWarpPhase()

Returns the current warp phase of the client sky, if warping. The following warp phases exist:

- `"slowingdown"`
- `"maintain"`
- `"speedingup"`

If called while the client sky is not warping, this will normally return `"slowingdown"`.
---

#### `float` celestial.skyWarpProgress()

Returns a value between `0.0` and `1.0` for how far through warping the sky is currently.
---

#### `bool` celestial.skyInHyperspace()

Returns whether the sky is currently under hyperspace flight.
---

#### `void` celestial.flyShip(`Vec3I` system, `SystemLocation` destination)

Flies the player ship to the specified `SystemLocation` in the specified system.

A `SystemLocation` must be one of the following types: `JsonNull`, `CelestialCoordinate`, `CelestialObject`, `Vec2F`.

The locations are specified as a pair of type and value. See the following Lua example:

```lua
local system = celestial.currentSystem().location
local system2 = jarray{100, -100, 200}
local location = nil -- `JsonNull`. I.e., a 'null' location.
location = jarray{"coordinate", jobject{location = system, planet = 1, satellite = 0}} -- `CelestialCoordinate`
-- `satellite` is 0 for planets or an integer above 0 for moons.
location = jarray{"coordinate", jobject{location = system2, planet = 3, satellite = 1}}
location = jarray{"object", "11112222333344445555666677778888"} -- `CelestialObject` (UUID)
location = jarray{0.0, 0.0} -- `Vec2F` (position in space)
celestial.flyShip(system, location)
```

---

#### `bool` celestial.flying()

Returns whether the player ship is flying.
---

#### `Maybe<Vec2F>` celestial.shipSystemPosition()

Returns the current position of the ship in the system.
---

#### `Maybe<SystemLocation>` celestial.shipDestination()

Returns the current destination of the player ship. Returns `nil` if the ship isn't currently moving. See `celestial.flyShip` for valid system location formats.
---

#### `Maybe<SystemLocation>` celestial.shipLocation()

Returns the current system location of the player ship. Returns `nil` if the ship is not currently at an actual location (e.g., if it's travelling). See `celestial.flyShip` for valid system location formats.
---

#### `CelestialCoordinate` celestial.currentSystem()

Returns the CelestialCoordinate for the system the ship is currently in. This is in the following form:

```lua
jobject{
  location = jarray{ -55876, 4229, 65429267 }, -- Integers specifying the celestial coordinate.
  planet = 0, -- Always 0.
  satellite = 0 -- Always 0.
}
```
---

#### `float` celestial.planetSize(`CelestialCoordinate` planet)

Returns the diameter of the specified planet in system space. Returns `0.0` if a nonexistent planet is specified.

**Note:** This callback returns `0.0` if the celestial database isn't loaded yet!
---

#### `Vec2F` celestial.planetPosition(`CelestialCoordinate` planet)

Returns the position of the specified planet in system space. Returns `jarray{0.0, 0.0}` if a nonexistent planet is specified.

**Note:** This callback returns `jarray{0.0, 0.0}` if the celestial database isn't loaded yet!
---

#### `Maybe<CelestialParameters>` celestial.planetParameters(`CelestialCoordinate` planet)

Returns the celestial parameters for the specified planet. The return value is in the following form:

```lua
jobject{
  seed = 554483824463574, -- 64-bit unsigned integer.
  name = "Denebola Expanse ^green;III^reset; - ^yellow;a^reset;", -- The planet's display name.
  parameters = jobject{}, -- A JSON object containing various world parameters. See the source code for details on these.
  visitableParameters = jobject{} -- The world's visitable parameters. See below. Is `nil` if the world isn't visitable.
}
```

Returns `nil` if a nonexistent planet is specified.

**Note:** This callback returns `nil` if the celestial database isn't loaded yet!
---

#### `Maybe<VisitableParameters>` celestial.visitableParameters(`CelestialCoordinate` planet)

Returns the visitable parameters for the specified visitable planet. For unvisitable or nonexistent planets, returns `nil`. Otherwise, the return value is in the following form:

```lua
jobject{
  typeName = "forest", -- The planet's type.
  threatLevel = 5.0, -- The world's threat level. Yes, this is a float.
  worldSize = jarray{600, 600}, -- The world's size in tiles.
  gravity = 1.0, -- The world's gravity level.
  airless = false, -- Whether the world is airless (i.e., you can't breathe without an O2 pack).
  weatherPool = jarray{"raining", "windy"}, -- The possible weather states on the world.
  environmentStatusEffects = jarray{"someStatusEffect"}, -- Environmental status effect names.
  overrideTech = jarray{}, -- Tech overrides. An empty array means no overrides are applied.
  -- If any entries are present, the world overrides player techs with the ones in this list,
  -- unless the player has `ignoreAllTechOverrides` enabled.
  globalDirectives = "?multiply=ff9", -- Any directives to visually apply to all sprites when a player is on the world.
  -- Don't know why nobody uses this one; it's present in stock Starbound.
  beamUpRule = "Surface", -- One of `"Nowhere"`, `"Surface"`, `"Anywhere"` or `"AnywhereWithWarning"`.
  -- Regulates whether and when the player is allowed to beam up from the world,
  -- and whether he gets a warning message when he tries to beam.
  disableDeathDrops = false, -- Whether to disable players dropping items on death while on the world.
  terraformed = false, -- Whether anyone ever used a terraformer on this world to change its type.
  worldEdgeForceRegions = "None" -- One of `"None"`, `"Top"`, `"Bottom"` or `"TopAndBottom"`.
  -- Specifies whether the bottom and/or top edges of a world have collision.
}
```
**Note:** This callback returns `nil` if the celestial database isn't loaded yet! Make sure the database is loaded before using this callback to check a world's visitability!
---

#### `Maybe<String>` celestial.planetName(`CelestialCoordinate` planet)

Returns the name of the specified planet. Returns `nil` if a nonexistent planet is specified.

**Note:** This callback returns `nil` if the celestial database isn't loaded yet!
---

#### `Maybe<uint64_t>` celestial.planetSeed(`CelestialCoordinate` planet)

Returns the seed for the specified planet. Returns `nil` if a system or nonexistent planet is specified.

**Note:** This callback returns `nil` if the celestial database isn't loaded yet!
---

#### `float` celestial.clusterSize(`CelestialCoordinate` planet)

Returns the diameter of the specified planet and its orbiting moons. If a moon or a star is passed to this callback, will return the cluster size for that system.
---

#### `Maybe<List<String>>` celestial.planetOres(`CelestialCoordinate` planet)

Returns a list of ores available on the specified planet. Returns `nil` if a system or nonexistent planet is specified.

**Note:** This callback returns `nil` if the celestial database isn't loaded yet!
---

#### `Vec2F` celestial.systemPosition(`SystemLocation` location)

Returns the position of the specified location in the *current system*.
---

#### `Vec2F` celestial.orbitPosition(`Orbit` orbit)

Returns the calculated position of the provided orbit.

```
local orbit = {
  target = planet, -- the orbit target
  direction = 1, -- orbit direction
  enterTime = 0, -- time the orbit was entered, universe epoch time
  enterPosition = {1, 0} -- the position that the orbit was entered at, relative to the target
}
```
---

#### `List<Uuid>` celestial.systemObjects()

Returns a list of the Uuids for objects in the current system.
---

#### `Maybe<String>` celestial.objectType(`Uuid` uuid)

Returns the type of the specified object. Returns `nil` if a nonexistent object is specified.
---

#### `Json` celestial.objectParameters(`Uuid` uuid)

Returns the parameters for the specified object. Returns `nil` if a nonexistent object is specified.
---

#### `Maybe<WorldId>` celestial.objectWarpActionWorld(`Uuid` uuid)

Returns the warp action world ID for the specified object. If an invalid warp action is specified, returns `nil`.
---

#### `Json` celestial.objectOrbit(`Uuid` uuid)

Returns the orbit of the specified object, if any. Otherwise returns `nil`.
---

#### `Maybe<Vec2F>` celestial.objectPosition(`Uuid` uuid)

Returns the position of the specified object, if any. Otherwise returns `nil`.
---

#### `Json` celestial.objectTypeConfig(`String` typeName)

Returns the configuration of the specified object type. Will throw a script error if an invalid object type is specified.
---

#### `Uuid` celestial.systemSpawnObject(`String` typeName, [`Vec2F` position], [`Uuid` uuid], [`Json` parameters])

Spawns an object of typeName at position. Optionally with the specified UUID and parameters.

If no position is specified, one is automatically chosen in a spawnable range.

Objects are limited to be spawned outside a distance of  `/systemworld.config:clientSpawnObjectPadding` from any planet surface (including moons), star surface, planetary orbit (including moons), or permanent objects orbits, and at most within `clientSpawnObjectPadding` from the outermost orbit.
---

#### `List<Uuid>` celestial.playerShips()

Returns a list of the player ships in the current system.
---

#### `Maybe<Vec2F>` celestial.playerShipPosition(`Uuid` uuid)

Returns the in-system position of the specified player ship, or `nil` if the ship is not in-system.
---

#### `Maybe<bool>` celestial.hasChildren(`CelestialCoordinate` coordinate)

Returns definitively whether the coordinate has orbiting children. `nil` return means the coordinate is not loaded.
---

#### `List<CelestialCoordinate>` celestial.children(`CelestialCoordinate` coordinate)

Returns the children for the specified celestial coordinate. For systems, returns planets, for planets, returns moons.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<int>` celestial.childOrbits(`CelestialCoordinate` coordinate)

Returns the child orbits for the specified celestial coordinate.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<CelestialCoordinate>` celestial.scanSystems(`RectI` region, [`Set<String>` includedTypes])

Returns a list of systems in the given region. If includedTypes is specified, this will return only systems whose typeName parameter is included in the set. This scans for systems asynchronously, meaning it may not return all systems if they have not been generated or sent to the client. Use `scanRegionFullyLoaded` to see if this is the case.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<pair<Vec2I, Vec2I>>` celestial.scanConstellationLines(`RectI` region)

Returns the constellation lines for the specified universe region.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `bool` celestial.scanRegionFullyLoaded(`RectI` region)

Returns whether the specified universe region has been fully loaded.
---

#### `List<pair<String, float>>` celestial.centralBodyImages(`CelestialCoordinate` system)

Returns the images with scales for the central body (star) for the specified system coordinate.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<pair<String, float>>` celestial.planetaryObjectImages(`CelestialCoordinate` coordinate)

Returns the smallImages with scales for the specified planet or moon.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<pair<String, float>>` celestial.worldImages(`CelestialCoordinate` coordinate)

Returns the generated world images with scales for the specified planet or moon.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

#### `List<pair<String, float>>` celestial.starImages(`CelestialCoordinate` system, `float` twinkleTime)

Returns the star image for the specified system. Requires a twinkle time to provide the correct image frame.

**Note:** This callback returns `jarray{}` if the celestial database isn't loaded yet!
---

## Versioning scripts

The following single `celestial` callback is available to JSON versioning scripts and is *not* part of the standard `celestial` callback table:
---

#### `CelestialParameters` celestial.parameters(`CelestialCoordinate` planet)

Returns the celestial parameters for the specified planet. The return value is in the following form:

```lua
jobject{
  seed = 554483824463574, -- 64-bit unsigned integer.
  name = "Denebola Expanse ^green;III^reset; - ^yellow;a^reset;", -- The planet's display name.
  parameters = jobject{}, -- A JSON object containing various world parameters. See the source code for details on these.
  visitableParameters = jobject{} -- The world's visitable parameters. See below. Is `nil` if the world isn't visitable.
}
```