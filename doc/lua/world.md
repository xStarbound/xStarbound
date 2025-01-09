# `world`

The `world` table contains bindings that perform actions within a specified world, such as querying or modifying entities, tiles, etc. in that world.

The `world` table is available in the following script contexts:

- universe client scripts (client-side, only on xStarbound)
- world context scripts (server-side)
- pane scripts (client-side)
- container interface scripts (client-side)
- monster, NPC and object scripts (client-side for client-mastered entities, server-side for server-mastered entities)
- monster and object animation scripts (client-side)
- stagehand scripts (server-side)
- generic player scripts (client-side)
- player companion scripts (client-side)
- player deployment scripts (client-side)
- quest scripts (client-side)
- tech scripts (client-side)
- status controller and effect scripts (client-side for client-mastered entities, server-side for server-mastered entities)
- vehicle scripts (client-side for client-mastered entities, server-side for server-mastered entities)
- projectile scripts (client-side for client-mastered projectiles, server-side for server-mastered projectiles)
- active and fireable item scripts (client-side for items used by client-mastered entities, server-side for items used by server-mastered entities)
- active item animation scripts (client-side)
- behaviour scripts (client-side if running on client-mastered entities, server-side if running on server-mastered entities)

The client-sided `world` callbacks are only available to client-sided scripts, while the server-sided callbacks are only available to server-sided scripts.

## Generic bindings

The following `world` bindings are available in both client- and server-side scripts.

---

#### `Json` world.getGlobal(`Maybe<String>` key)
#### `Json` world.getGlobals()

> **Available only on xStarbound.**

Returns a global script variable (if `getGlobal` is called with a specified string key) or a JSON object containing all of them (if `getGlobal` is called with no key, or `getGlobals` is called). Global script variables may be any arbitrary JSON value.

On xClient, these variables are shared across all client-side script contexts where the `world` table is available, and persist until disconnection. On xServer, these variables are shared across all script contexts running on a given world, and persist until that world is unloaded.

**Note:** These callbacks are a safer preferred alternative to using `shared` and smuggling global context variables through metatables and library tables.

---

#### `Json` world.setGlobal(`Maybe<String>` key, `Json` value)
#### `Json` world.setGlobals(`JsonObject` newGlobals)

> **Available only on xStarbound.**

Sets a global script variable (if `setGlobal` is called with a specified string key instead of `nil`) or all of them as a single JSON object (if `setGlobal` is called with a `nil` key, or `setGlobals` is called). Global script variables may be any arbitrary JSON value. If the entire globals object is being set at once, any value other than a JSON object is ignored.

On xClient, these variables are shared across all client-side script contexts where the `world` table is available, and persist until disconnection. On xServer, these variables are shared across all script contexts running on a given world, and persist until that world is unloaded.

**Note:** These callbacks are a safer preferred alternative to using `shared` and smuggling global context variables through metatables and library tables, which is disallowed on xStarbound when `"safeScripts"` is enabled.

---

#### `String` world.type()

Returns a string describing the world's type. For terrestrial worlds this will be the primary biome, for instance worlds this will be the instance name, and for ship or generic worlds this will be `"unknown"`.

---

#### `bool` world.terrestrial()

Returns a `true` if the current world is a terrestrial world, i.e., a planet, or `false` otherwise.

---

#### `Vec2I` world.size()

Returns a vector describing the size of the current world in world tiles.

---

#### `float` world.magnitude(`Vec2F` position1, `Vec2F` position2)

Returns the magnitude of the distance between the specified world positions. Use this rather than simple vector subtraction to handle world wrapping.

---

#### `Vec2F` world.distance(`Vec2F` position1, `Vec2F` position2)

Returns the vector difference between the specified world positions. Use this rather than simple vector subtraction to handle world wrapping.

---

#### `bool` world.polyContains(`PolyF` poly, `Vec2F` position)

Returns `true` if the specified poly contains the specified position in world space and `false` otherwise.

---

#### `Vec2F` world.xwrap(`Vec2F` position)

Returns the specified position with its X coordinate wrapped around the world width.

#### `float` world.xwrap(`float` xPosition)

Returns the specified X position wrapped around the world width.

---

#### `Variant<Vec2F, float>` world.nearestTo(`Variant<Vec2F, float>` sourcePosition, `Variant<Vec2F, float>` targetPosition)

Returns the point nearest to (i.e. on the same side of the world as) the source point. Either argument can be specified as a `Vec2F` point or as a `float` X position. The type of the targetPosition determines the return type.

---

#### `bool` world.pointCollision(`Vec2F` point, [`CollisionSet` collisionKinds])

Returns `true` if the generated collision geometry at the specified point matches any of the specified collision kinds and `false` otherwise.

---

#### `bool` world.pointTileCollision(`Vec2F` point, [`CollisionSet` collisionKinds])

Returns `true` if the tile at the specified point matches any of the specified collision kinds and `false` otherwise.

---

#### `Tuple<Maybe<Vec2F>, Maybe<Vec2F>>` world.lineCollision(`Vec2F` startPoint, `Vec2F` endPoint, [`CollisionSet` collisionKinds])

If the line between the specified points overlaps any generated collision geometry of the specified collision kinds, returns the point at which the line collides, or `nil` if the line does not collide. If intersecting a side of the poly, also returns the normal of the intersected side as a second return.

---

#### `bool` world.lineTileCollision(`Vec2F` startPoint, `Vec2F` endPoint, [`CollisionSet` collisionKinds])

Returns `true` if the line between the specified points overlaps any tiles of the specified collision kinds and `false` otherwise.

---

#### `Maybe<pair<Vec2F, Vec2F>>` world.lineTileCollisionPoint(`Vec2F` startPoint, `Vec2F` endPoint, [`CollisionSet` collisionKinds])

Returns a table of `{position, normal}` where `position` is the position that the line intersects the first collidable tile, and `normal` is the collision normal. Returns `nil` if no tile is intersected.

---

#### `bool` world.rectCollision(`RectF` rect, [`CollisionSet` collisionKinds])

Returns `true` if the specified rectangle overlaps any generated collision geometry of the specified collision kinds and `false` otherwise.

---

#### `bool` world.rectTileCollision(`RectF` rect, [`CollisionSet` collisionKinds])

Returns `true` if the specified rectangle overlaps any tiles of the specified collision kinds and `false` otherwise.

---

#### `bool` world.polyCollision(`PolyF` poly, [`Vec2F` position], [`CollisionSet` collisionKinds])

Returns `true` if the specified polygon overlaps any generated collision geometry of the specified collision kinds and `false` otherwise. If a position is specified, the polygon coordinates will be treated as relative to that world position.

---

#### `List<Vec2I>` world.collisionBlocksAlongLine(`Vec2F` startPoint, `Vec2F` endPoint, [`CollisionSet` collisionKinds], [`int` maxReturnCount])

Returns an ordered list of tile positions along the line between the specified points that match any of the specified collision kinds. If maxReturnCount is specified, the function will only return up to that number of points.

---

#### `List<pair<Vec2I, LiquidLevel>>` world.liquidAlongLine(`Vec2F` startPoint, `Vec2F` endPoint)

Returns a list of pairs containing a position and a `LiquidLevel` for all tiles along the line between the specified points that contain any liquid.

---

#### `Vec2F` world.resolvePolyCollision(`PolyF` poly, `Vec2F` position, `float` maximumCorrection, [`CollisionSet` collisionKinds])

Attempts to move the specified poly (relative to the specified position) such that it does not collide with any of the specified collision kinds. Will only move the poly up to the distance specified by maximumCorrection. Returns `nil` if the collision resolution fails.

---

#### `bool` world.tileIsOccupied(`Vec2I` tilePosition, [`bool` foregroundLayer], [`bool` includeEphemeral])

Returns `true` if the specified tile position is occupied by a material or tile entity and `false` if it is empty. The check will be performed on the foreground tile layer if `foregroundLayer` is `true` (or unspecified) or the background tile layer if it is `false`. The check will include ephemeral tile entities such as preview objects if includeEphemeral is `true`, and will not include these entities if it is `false` (or unspecified).

---

#### `bool` world.placeObject(`String` objectName, `Vec2I` tilePosition, [`int` direction], [`Json` parameters])

Attempts to place the specified object into the world at the specified position, preferring it to be right-facing if direction is positive (or unspecified) and left-facing if it is negative. If parameters are specified they will be applied to the object. Returns `true` if the object is placed successfully and `false` otherwise.

---

#### `EntityId` world.spawnItem(`ItemDescriptor` item, `Vec2F` position, [`unsigned` count], [`Json` parameters], [`Vec2F` velocity], [`float` intangibleTime])

Attempts to spawn the specified item into the world as the specified position. If item is specified as a name, it will optionally apply the specified count and parameters. The item drop entity can also be spawned with an initial velocity and intangible time (delay before it can be picked up) if specified. Returns an `EntityId` of the item drop if successful and `nil` otherwise. If a `nil` item descriptor is specified, nothing is spawned.

---

#### `List<EntityId>` world.spawnTreasure(`Vec2F` position, `String` poolName, `float` level, [`unsigned` seed])

Attempts to spawn all items in an instance of the specified treasure pool with the specified level and seed at the specified world position. Returns a list of `EntityId`s of the item drops created if successful and `nil` otherwise.

---

#### `EntityId` world.spawnMonster(`String` monsterType, `Vec2F` position, [`Json` parameters])

Attempts to spawn a monster of the specified type at the specified position. If parameters are specified they will be applied to the spawned monster. If they are unspecified, they default to an object setting aggressive to be randomly `true` or `false`. Level for the monster may be specified in parameters. Returns the `EntityId` of the spawned monster if successful and `nil` otherwise.

---

#### `EntityId` world.spawnNpc(`Vec2F` position, `String` species, `String` npcType, `float` level, [`unsigned` seed], [`Json` parameters])

Attempts to spawn an NPC of the specified type, species, level with the specified seed and parameters at the specified position. Returns `EntityId` of the spawned NPC if successful and `nil` otherwise.

---

#### `EntityId` world.spawnStagehand(`Vec2F` position, `String` type, [`Json` overrides])

Attempts to spawn a stagehand of the specified type at the specified position with the specified override parameters. Returns `EntityId` of the spawned stagehand if successful and `nil` otherwise.

---

#### `EntityId` world.spawnProjectile(`String` projectileName, `Vec2F` position, [`EntityId` sourceEntityId], [`Vec2F` direction], [`bool` trackSourceEntity], [`Json` parameters])

Attempts to spawn a projectile of the specified type at the specified position with the specified source entity ID, direction, and parameters. If trackSourceEntity is `true` then the projectile's position will be locked relative to its source entity's position. Returns the `EntityId` of the spawned projectile if successful and `nil` otherwise.

---

#### `EntityId` world.spawnVehicle(`String` vehicleName, `Vec2F` position, [`Json` overrides])

Attempts to spawn a vehicle of the specified type at the specified position with the specified override parameters. Returns the `EntityId` of the spawned vehicle if successful and `nil` otherwise.

---

#### `float` world.threatLevel()

Returns the threat level of the current world.

---

#### `double` world.time()

Returns the absolute time of the current world.

---

#### `unsigned` world.day()

Returns the absolute numerical day of the current world.

---

#### `double` world.timeOfDay()

Returns a value between 0 and 1 indicating the time within the day of the current world.

---

#### `float` world.dayLength()

Returns the duration of a day on the current world.

---

### `Maybe<String>` world.biomeAt(`Vec2I` tilePos, `Maybe<bool>` getBlockBiome)

> **Only available on xStarbound.**

Returns the biome at the given world tile position for this world, or `nil` if there's no biome there. If `getBlockBiome` is `false` or unspecified, this returns the biome that affects the visible *parallax*; if `true`, this returns any biome or sub-biome at the location, even if it *doesn't* affect parallax.

---

### `Maybe<JsonObject>` world.biomeParametersAt(`Vec2I` tilePos, `Maybe<bool>` getBlockBiome)

> **Only available on xStarbound.**

Similar to `world.biomeAt` above, but returns a JSON object containing the parameters for any biome at the specified location, or `nil`/`null` if there's no biome there. `getBlockBiome` functions as it does for `world.biomeAt`.

---

#### `Json` world.getProperty(`String` propertyName, [`Json` defaultValue])

Returns the JSON value of the specified world property, or defaultValue or `nil` if it is not set.

---

#### `void` world.setProperty(`String` propertyName, `Json` value)

Sets the specified world property to the specified value.

---

#### `LiquidLevel` world.liquidAt(`Vec2I` position)

Returns the `LiquidLevel` at the specified tile position, or `nil` if there is no liquid.

#### `LiquidLevel` world.liquidAt(`RectF` region)

Returns the average `LiquidLevel` of the most plentiful liquid within the specified region, or `nil` if there is no liquid.

---

#### `float` world.gravity(`Vec2F` position)

Returns the gravity at the specified position. This should be consistent for all non-dungeon tiles in a world but can be altered by dungeons.

---

#### `bool` world.spawnLiquid(`Vec2F` position, `LiquidId` liquid, `float` quantity)

Attempts to place the specified quantity of the specified liquid at the specified position. Returns `true` if successful and `false` otherwise.

---

#### `LiquidLevel` world.destroyLiquid(`Vec2F` position)

Removes any liquid at the specified position and returns the LiquidLevel containing the type and quantity of liquid removed, or `nil` if no liquid is removed.

---

#### `bool` world.isTileProtected(`Vec2F` position)

Returns `true` if the tile at the specified position is protected and `false` otherwise.

---

#### `List<Json>` world.findPlatformerPath(`Vec2F` startPosition, `Vec2F` endPosition, `JsonObject` movementParameters, `JsonObject` searchParameters)

Attempts to synchronously pathfind between the specified positions using the specified movement and pathfinding parameters. Returns the path as a list of nodes as successful, or `nil` if no path is found.

The `movementParameters` are standard actor movement parameters. See `$xSBassets/scripts/pathing.lua` for examples of the `searchParameters`.

> *On xStarbound, enabling `multiJump` in the `airJumpProfile` within `movementParameters` allows the A\* platformer to use mid-air jumps in pathing.*

Example of a returned list of path nodes:

```lua
jarray{
    jobject{
        cost = 12.0 -- The cost of the action. A `float`.
        action = "Walk" -- The action type. Is one of `"Walk"`, `"Jump"`, `"Arc"`,
        -- `"Drop"`, `"Swim"`, `"Fly"` or `"Land"`.
        jumpVelocity = 25.0 -- For jumps (but not mid-air arcs), the jump velocity.
        source = jobject{ -- The source node where this action is expected to start.
            position = {346.233, 567.234}.
            velocity = {0.0, -12.332}
        },
        target = jobject{ -- The target node where this action is expected to end.
            position = {346.233, 557.25}.
            velocity = {0.0, 0.0}
        }
    },
    ...
}
```

---

#### `PathFinder` world.platformerPathStart(`Vec2F` startPosition, `Vec2F` endPosition, `JsonObject` movementParameters, `JsonObject` searchParameters)

Creates and returns a Lua userdata value which can be used for pathfinding over multiple frames. 

The `movementParameters` are standard actor movement parameters. See `$xSBassets/scripts/pathing.lua` for examples of the `searchParameters`.

> *On xStarbound, enabling `multiJump` in the `airJumpProfile` within `movementParameters` allows the A\* platformer to use mid-air jumps in pathing.*

The `PathFinder` returned has the following two methods:

##### `bool` explore([`int` nodeLimit])

Explores the path up to the specified node count limit. Returns `true` if the pathfinding is complete and `false` if it is still incomplete. If nodeLimit is unspecified, this will search up to the configured maximum number of nodes, making it equivalent to world.platformerPathStart.

##### `List<Json>` result()

Returns the completed path. This is in the format shown under `world.findPlatformerPath`.

---

#### `float` world.lightLevel(`Vec2F` position)

Returns the current logical light level at the specified position. Requires recalculation of lighting, so this should be used sparingly.

---

#### `float` world.windLevel(`Vec2F` position)

Returns the current wind level at the specified position.

---

#### `bool` world.breathable(`Vec2F` position)

Returns `true` if the world is breathable at the specified position and `false` otherwise.

---

#### `List<String>` world.environmentStatusEffects(`Vec2F` position)

Returns a list of the environmental status effects at the specified position.

---

#### `bool` world.underground(`Vec2F` position)

Returns `true` if the specified position is below the world's surface level and `false` otherwise.

---

#### `bool` world.inSurfaceLayer(`Vec2I` position)

Returns `true` if the world is terrestrial and the specified position is within its surface layer, and `false` otherwise.

> **Warning for non-xStarbound modders:** This callback causes an immediate segfault when invoked with a negative Y coordinate on OpenStarbound and stock Starbound. The bug has been fixed on xStarbound.

---

#### `float` world.surfaceLevel()

Returns the surface layer base height.

---

#### `int` world.oceanLevel(`Vec2I` position)

If the specified position is within a region that has ocean (endless) liquid, returns the world Y level of that ocean's surface, or `0` if there is no ocean in the specified region.

---

#### `Variant<String, bool>` world.material(`Vec2F` position, `String` layerName)

Returns the name of the material at the specified position and layer. Layer can be specified as 'foreground' or 'background'. Returns `false` if the space is empty in that layer. Returns `nil` if the material is NullMaterial (e.g. if the position is in an unloaded sector).

---

#### `String` world.mod(`Vec2F` position, `String` layerName)

Returns the name of the mod at the specified position and layer, or `nil` if there is no mod.

---

#### `float` world.materialHueShift(`Vec2F` position, `String` layerName)

Returns the hue shift of the material at the specified position and layer.

---

#### `float` world.modHueShift(`Vec2F` position, `String` layerName)

Returns the hue shift of the mod at the specified position and layer.

---

#### `unsigned` world.materialColor(`Vec2F` position, `String` layerName)

Returns the colour variant (i.e., painted colour) of the material at the specified position and layer.

---

#### `void` world.setMaterialColor(`Vec2F` position, `String` layerName, `unsigned` color)

Sets the colour variant of the material at the specified position and layer to the specified colour.

---

#### `bool` world.damageTiles(`List<Vec2I>` positions, `String` layerName, `Vec2F` sourcePosition, `String` damageType, `float` damageAmount, [`unsigned` harvestLevel], [`EntityId` sourceEntity])

Damages all tiles in the specified layer and positions by the specified amount. The source position of the damage determines the initial direction of the damage particles. Damage types are: `"plantish"`, `"blockish"`, `"beamish"`, `"explosive"`, `"fire"`, `"tilling"`. Harvest level determines whether destroyed materials or mods will drop as items. Returns `true` if any damage was done and `false` otherwise.

> **Technical note:** Tiles with a configured `"health"` value of infinity (`math.huge` in Lua) can only be destroyed with an infinite amount of damage (again, `math.huge` in Lua). The only way to set such a `"health"` value is with a Lua patch script (*not* a preprocessor script!), as this is the only way to set any value *after* text parsing, which considers any `inf` or `Infinity` values invalid (as per the JSON standard).

---

#### `bool` world.damageTileArea(`Vec2F` center, `float` radius, `String` layerName, `Vec2F` sourcePosition, `String` damageType, `float` damageAmount, [`unsigned` harvestLevel], [`EntityId` sourceEntity])

Identical to `world.damageTiles`, but applies to tiles in a circular radius around the specified center point.

---

#### `List<Vec2I>` world.radialTileQuery(`Vec2F` position, `float` radius, `String` layerName)

Returns a list of existing tiles within `radius` of the given position, on the specified tile layer.

---

#### `bool` world.placeMaterial(`Vec2I` position, `String` layerName, `String` materialName, [`int` hueShift], [`bool` allowOverlap], [`bool` allowDisconnected])
#### `bool` world.placeMaterial(`Vec2I` position, `String` layerName, `String` materialName, [`int` hueShift], [`bool` allowOverlap])

> *`allowDisconnected` is only available on xStarbound, and layer names are only available on xStarbound and OpenStarbound.*

Attempts to place the specified material in the specified position and layer. If `allowOverlap` is `true`, the material can be placed in a space occupied by mobile entities, otherwise such placement attempts will fail. If `allowDisconnected` is `true`, the material does not need to be connected to any other tiles to be placed. Returns `true` if the placement succeeds and `false` otherwise.

> **Note on `allowDisconnected`:** The client-side use of `allowDisconnected` to ignore the requirement for newly placed tiles to be connected to existing tiles currently requires *both* xClient *and* xServer (although this may change), or only xClient if you're in single-player. Server-side use of `allowDisconnected` only requires xServer.

> **Note on `layerName`:** The `layerName` may optionally include any one of the following collision modifiers (e.g. `"foreground+none"`, `"background+block"`):
>
> - `+none`: The tile is placed with no collision. Entities can move through the tile regardless of whether collision is active in their movement controllers, and objects can be placed in front of or behind the tile, depending on its layer. However, entities with active collision cannot stand on the tile, nor can objects be anchored to it.
> - `+platform`: The tile is placed with platform collision. Entities can move through the tile regardless of whether collision is active in their movement controllers, but objects cannot be placed in front of or behind the tile. Entities with active collision can stand on the tile and objects can be anchored to it, provided there is enough space around the tile.
> - `+block`: The tile is placed with block collision. Entities cannot move through the tile unless collision is disabled in their movement controllers, nor can objects be placed in front or behind the tile. Entities with active collision can stand on the tile and objects can be anchored to it, provided there is enough space around the tile.
>
> Collision modifiers affect only foreground tiles; background tiles always have no collision. To see what kind of tile collision a given foreground tile has, enable `/debug` and `/boxes`. If no collision modifier is specified, the default configured collision for the material type is used when placing foreground tiles; this is normally platform collision for platforms and block collision for other tiles.
>
> Collision modifiers are currently supported on xServer and OpenStarbound servers. StarExtensions collision modifiers are network-compatible with xServer v2.0.0+ (and OpenStarbound) servers, although StarExtensions provides no modified `world.placeMaterial` callback for this.

> **Tip:** Use [xWEdit](https://github.com/FezzedOne/xWEdit) if you want WEdit support for all the new features of `placeMaterial`.

---

#### `bool` world.placeMod(`Vec2I` position, `String` layerName, `String` modName, [`int` hueShift], [`bool` allowOverlap])

Attempts to place the specified mod in the specified position and layer. If allowOverlap is `true` the mod can be placed in a space occupied by mobile entities, otherwise such placement attempts will fail. Returns `true` if the placement succeeds and `false` otherwise.

---

#### `List<EntityId>` world.entityQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`LuaTable` options])

Queries for entities in a specified area of the world and returns a list of their entity IDs. Area can be specified either as the `Vec2F` lower left and upper right positions of a rectangle, or as the `Vec2F` center and `float` radius of a circular area. The following additional parameters can be specified in the `options` table:

- __withoutEntityId__ - Specifies an `EntityId` that will be excluded from the returned results
- __includedTypes__ - Specifies a list of one or more `String` entity types that the query will return. The valid entity type names are:
  - `"plant"`
  - `"object"`
  - `"vehicle"`
  - `"itemDrop"`
  - `"plantDrop"`
  - `"projectile"`
  - `"stagehand"`
  - `"monster"`
  - `"npc"`
  - `"player"`

  In addition to standard entity type names, this list can include `"mobile"` for all mobile entity types or `"creature"` for players, monsters and NPCs.
- __boundMode__ - Specifies the bounding mode for determining whether entities fall within the query area. Valid options are `"position"`, `"collisionarea"` and `"metaboundbox"`. Defaults to `"collisionarea"` if unspecified.
- __order__ - A `String` used to specify how the results will be ordered. If this is set to `"nearest"` the entities will be sorted by ascending distance from the first positional argument. If this is set to `"random"` the list of results will be shuffled. If not specified, the list of entities will be in an undefined order.
- __callScript__ - Specifies a `String` name of a function that should be called in the script context of all scripted entities matching the query. Accepts Lua dot notation.
- __callScriptArgs__ - Specifies a list of arguments — `Json` values if `"safeScripts"` is enabled on xStarbound, or `LuaValue`s otherwise — that will be passed to the function called by `callScript`.
- __callScriptResult__ - Specifies a value — `Json` if `"safeScripts"` is enabled on xStarbound, or a `LuaValue` otherwise — that the function called by callScript must return; entities whose script calls do not return this value will be excluded from the results. Defaults to `true`. On xStarbound, `null` or `json.null` may be passed if you want to make sure the called function returns a `nil` or nothing.

On xStarbound with `"safeScripts"` enabled, all `callScript` arguments and the `callScriptResult` must be valid JSON, and an error will be thrown after the first script call if the returned result isn't convertible to valid JSON before comparison happens.

**Warning:** If `"safeScripts"` is disabled on xStarbound, and regardless of this on other Starbound servers and clients, potentially unsafe Lua values can be passed through `callScriptArgs` and `callScriptResult`. If unsafe passing is allowed, you should avoid passing Lua bindings or anything that can call them. Calling entity bindings after the entity has been removed from the game *will* almost certainly cause segfaults or memory corruption!

---

#### `List<EntityId>` world.monsterQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Identical to `world.entityQuery`, but only considers monsters.

---

#### `List<EntityId>` world.npcQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Identical to `world.entityQuery`, but only considers NPCs.

---

#### `List<EntityId>` world.objectQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Similar to `world.entityQuery`, but only considers objects. Allows an additional option, __name__, which specifies a `String` object type name and will only return objects of that type.

---

#### `List<EntityId>` world.itemDropQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Identical to `world.entityQuery`, but only considers item drops.

---

#### `List<EntityId>` world.playerQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Identical to `world.entityQuery`, but only considers players.

---

#### `List<EntityId>` world.loungeableQuery(`Vec2F` position, `Variant<Vec2F, float` positionOrRadius, [`Json` options])

Similar to world.entityQuery but only considers loungeable entities. Allows an additional option, __orientation__, which specifies the `String` name of a loungeable orientation ("sit", "lay" or "stand") and only returns loungeable entities which use that orientation.

---

#### `List<EntityId>` world.entityLineQuery(`Vec2F` startPosition, `Vec2F` endPosition, [`Json` options])

Similar to world.entityQuery but only returns entities that intersect the line between the specified positions.

---

#### `List<EntityId>` world.objectLineQuery(`Vec2F` startPosition, `Vec2F` endPosition, [`Json` options])

Identical to world.entityLineQuery but only considers objects.

---

#### `List<EntityId>` world.npcLineQuery(`Vec2F` startPosition, `Vec2F` endPosition, [`Json` options])

Identical to world.entityLineQuery but only considers NPCs.

---

#### `EntityId` world.objectAt(`Vec2I` tilePosition)

Returns the entity ID of any object occupying the specified tile position, or `nil` if the position is not occupied by an object.

---

#### `bool` world.entityExists(`EntityId` entityId)

Returns `true` if an entity with the specified ID exists in the world and `false` otherwise.

---

#### `DamageTeam` world.entityDamageTeam(`EntityId` entityId)

Returns the current damage team (team type and team number) of the specified entity, or `nil` if the entity doesn't exist.

---

#### `bool` world.entityCanDamage(`EntityId` sourceId, `EntityId` targetId)

Returns `true` if the specified source entity can damage the specified target entity using their current damage teams and `false` otherwise.

---

#### `bool` world.entityAggressive(`EntityId` entity)

Returns `true` if the specified entity is an aggressive monster or NPC, or `false` otherwise.

---

#### `Maybe<String>` world.entityType(`EntityId` entityId)

Returns the entity type name of the specified entity, or `nil` if the entity doesn't exist. The valid entity type names are:

- `"plant"`
- `"object"`
- `"vehicle"`
- `"itemDrop"`
- `"plantDrop"`
- `"projectile"`
- `"stagehand"`
- `"monster"`
- `"npc"`
- `"player"`

---

#### `Maybe<Vec2F>` world.entityPosition(`EntityId` entityId)

Returns the current world position of the specified entity, or `nil` if the entity doesn't exist.

---

#### `Maybe<Vec2F>` world.entityMouthPosition(`EntityId` entityId)

Returns the current world mouth position of the specified player, monster, NPC or object, or `nil` if the entity doesn't exist or isn't a valid type.

---

#### `Maybe<Vec2F>` world.entityVelocity(`EntityId` entityId)

Returns the current velocity of the entity if it is a vehicle, monster, NPC or player and `nil` otherwise.

---

#### `Maybe<RectF>` world.entityMetaBoundBox(`EntityId` entityId)

Returns the meta bound box of the specified entity, if any. This bound box is used by the game as the bound box for entity interactions and chunkloading. In regards to entity interactions, a middle-click or **Interact** bind press within an entity's meta bound box, provided it is within the player's interaction range and the entity is marked as interactive (*or* provided that `/admin` or overreach mode is enabled), will trigger an interaction with the entity, notwithstanding other conflicting meta bound boxes partially or completely inside the returned one.

---

#### `Maybe<unsigned>` world.entityCurrency(`EntityId` entityId, `String` currencyType)

Returns the specified player entity's stock of the specified currency type, or `nil` if the entity is not a player.

---

#### `Maybe<unsigned>` world.entityHasCountOfItem(`EntityId` entityId, `Json` itemDescriptor, [`bool` exactMatch])

Returns the total count (sum of the stack counts of all matching items) of the specified item in the specified player's inventory, `0` if no matching items (or stacks) are found, or `nil` if the entity is not a player. If `exactMatch` is `true`, both the item name and parameters in the specified descriptor must match a given item stack in order for that stack to be counted; if `false`, only the item name needs to match for a given item stack to be counted, and any parameters are permissible.

> **Note:** If the specified player has any inventory slots or bags that aren't networked, any matching items in these «hidden» slots will *not* be counted.

---

#### `Maybe<Vec2F>` world.entityHealth(`EntityId` entityId)

Returns a `Vec2F` containing the specified entity's current and maximum health, as the first and second elements, respectively, if the entity is a player, monster or NPC, or `nil` otherwise.

---

#### `Maybe<String>` world.entitySpecies(`EntityId` entityId)

Returns the name of the specified entity's species if it is a player or NPC, or `nil` otherwise.

---

#### `Maybe<String>` world.entityGender(`EntityId` entityId)

Returns the name of the specified entity's gender if it is a player or NPC, or `nil` otherwise.

---

#### `Maybe<String>` world.entityName(`EntityId` entityId)

Returns a `String` name of the specified entity. This callback has different behaviour for different entity types. For players, monsters and NPCs, this will be the configured name of the specific entity. For objects, vehicles or stagehands, this will be the name of the object, vehicle or stagehand type. For item drops, this will be the name of the contained item. Returns `nil` if the specified entity is a plant or plant drop.

---

#### `Maybe<String>` world.entityTypeName(`EntityId` entityId)

Similar to `world.entityName` but returns the names of configured types for NPCs and monsters. Returns `nil` if the specified entity is a player, plant, plant drop or stagehand.

---

#### `Maybe<String>` world.entityDescription(`EntityId` entityId, [`String` species])

Returns the configured description for the specified inspectable entity (currently only objects and plants support this). Will return a species-specific description if a species name is specified, or a generic description otherwise.

---

#### `Maybe<JsonArray>` world.entityPortrait(`EntityId` entityId, `String` portraitMode)

Generates a portrait of the specified entity in the specified portrait mode and returns a list of drawables, or `nil` if the entity is not a portrait entity. `portraitMode` only affects portraits of players and NPCs. The available modes are as follows:

- `"head"`: Just the entity's head and a sliver of the entity's upper body. `?crop` directives will be added to the portrait drawables.
- `"bust"`: The entity's head and the upper part of the entity's body. `?crop` directives will be added to the portrait drawables.
- `"full"`: The entity's full body.
- `"fullneutral"`: The player's full body, posed «neutrally» in the species' first configured personality (normally `idle.1`).
- `"fullnude"`: The player's full body, without any armour or clothing. Note that any «clothing» in the form of modified humanoid directives is still rendered.
- `"fullneutralnude"`: The player's full body, posed «neutrally» *and* without clothing (as above).

---

#### `Maybe<String>` world.entityHandItem(`EntityId` entityId, `String` handName)

Returns the name of the item held in the specified hand of the specified player or NPC, or `nil` if the entity is not holding an item or is not a player or NPC. Hand name should be specified as `"primary"` (left hand; two-handed items also go here) or `"alt"` (right hand).

---

#### `Maybe<Vec2F>` world.entityAimPosition(`EntityId` entityId)

**Only available on xStarbound and OpenStarbound.**

Returns the current aim position of this entity, or `nil` if the entity is not a player or NPC.

---

#### `Maybe<ItemDescriptor>` world.entityHandItemDescriptor(`EntityId` entityId, `String` handName)

Similar to `world.entityHandItem`, but returns the full descriptor of the item rather than the name.

---

#### `Maybe<ItemDescriptor>` world.itemDropItem(`EntityId` entityId)

Returns the item descriptor of an item drop's contents, or `nil` if the specified entity is not an item drop.

---

#### `Maybe<List<MaterialId>>` world.biomeBlocksAt(`Vec2I` position)

Returns the list of biome specific blocks that can place in the biome at the specified position.

---

#### `Maybe<String>` world.entityUniqueId(`EntityId` entityId)

Returns the unique ID of the specified entity, or `nil` if the entity does not have a unique ID. Will return a vanity UUID if the entity uses such.

---

#### `Json` world.getNpcScriptParameter(`EntityId` entityId, `String` parameterName, [`Json` defaultValue])

Returns the value of the specified NPC's variant script config parameter, or defaultValue or `nil` if the parameter is not set or the entity is not an NPC.

---

#### `Json` world.getObjectParameter(`EntityId` entityId, `String` parameterName, [`Json` defaultValue])

Returns the value of the specified object's config parameter, or defaultValue or `nil` if the parameter is not set or the entity is not an object.

---

#### `List<Vec2I>` world.objectSpaces(`EntityId` entityId)

Returns a list of tile positions that the specified object occupies, or `nil` if the entity is not an object.

---

#### `Maybe<int>` world.farmableStage(`EntityId` entityId)

Returns the current growth stage of the specified farmable object, or `nil` if the entity is not a farmable object.

---

#### `Maybe<int>` world.containerSize(`EntityId` entityId)

Returns the total capacity of the specified container, or `nil` if the entity is not a container.

---

#### `bool` world.containerClose(`EntityId` entityId)

Visually closes the specified container. Returns `true` if the entity is a container and `false` otherwise.

---

#### `bool` world.containerOpen(`EntityId` entityId)

Visually opens the specified container. Returns `true` if the entity is a container and `false` otherwise.

---

#### `Maybe<JsonArray>` world.containerItems(`EntityId` entityId)

Returns a list of pairs of item descriptors and container positions of all items in the specified container, or `nil` if the entity is not a container.

---

#### `Maybe<ItemDescriptor>` world.containerItemAt(`EntityId` entityId, `unsigned` offset)

Returns an item descriptor of the item at the specified position in the specified container, or `nil` if the entity is not a container or the offset is out of range.

---

#### `bool` world.containerConsume(`EntityId` entityId, `ItemDescriptor` item)

Attempts to consume items from the specified container that match the specified item descriptor and returns `true` if successful, `false` if unsuccessful, or `nil` if the entity is not a container. Only succeeds if the full count of the specified item can be consumed.

---

#### `bool` world.containerConsumeAt(`EntityId` entityId, `unsigned` offset, `unsigned` count)

Similar to `world.containerConsume`, but only considers the specified slot within the container.

---

#### `Maybe<unsigned>` world.containerAvailable(`EntityId` entityId, `ItemDescriptor` item)

Returns the total count (sum of all stack counts) of the specified item that is currently available to consume in the specified container, `0` if no matching items are found, or `nil` if the entity is not a container. Both the item name and parameters must match in order for a given item stack to be counted.

---

#### `JsonArray` world.containerTakeAll(`EntityId` entityId)

Similar to `world.containerItems` but also consumes all items in the container.

---

#### `Maybe<ItemDescriptor>` world.containerTakeAt(`EntityId` entityId, `unsigned` offset)

Similar to `world.containerItemAt`, but consumes all items in the specified slot of the container.

---

#### `Maybe<ItemDescriptor>` world.containerTakeNumItemsAt(`EntityId` entityId, `unsigned` offset, `unsigned` count)

Similar to `world.containerTakeAt`, but consumes up to (but not necessarily equal to) the specified count of items from the specified slot of the container and returns only the items consumed.

---

#### `Maybe<unsigned>` world.containerItemsCanFit(`EntityId` entityId, `ItemDescriptor` item)

Returns the number of times the specified item can fit in the specified container, or `nil` if the entity is not a container.

---

#### `Json` world.containerItemsFitWhere(`EntityId` entityId, `ItemDescriptor` items)

Returns a `JsonObject` containing a list of `"slots"` where the specified item would fit and the count of `"leftover"` items that would remain after attempting to add the items. Returns `nil` if the entity is not a container.

---

#### `ItemDescriptor` world.containerAddItems(`EntityId` entityId, `ItemDescriptor` items)

Adds the specified items to the specified container. Returns the leftover items after filling the container, `nil` if none are left over, or all items if the entity is not a container.

---

#### `ItemDescriptor` world.containerStackItems(`EntityId` entityId, `ItemDescriptor` items)

Similar to `world.containerAddItems` but will only combine items with existing stacks and will not fill empty slots.

---

#### `ItemDescriptor` world.containerPutItemsAt(`EntityId` entityId, `ItemDescriptor` items, `unsigned` offset)

Similar to `world.containerAddItems` but only considers the specified slot in the container.

---

#### `ItemDescriptor` world.containerItemApply(`EntityId` entityId, `ItemDescriptor` items, `unsigned` offset)

Attempts to combine the specified items with the current contents (if any) of the specified container slot and returns any items unable to be placed into the slot.

---

#### `ItemDescriptor` world.containerSwapItemsNoCombine(`EntityId` entityId, `ItemDescriptor` items, `unsigned` offset)

Places the specified items into the specified container slot and returns the previous contents of the slot if successful, or the original items if unsuccessful.

---

#### `ItemDescriptor` world.containerSwapItems(`EntityId` entityId, `ItemDescriptor` items, `unsigned` offset)

A combination of `world.containerItemApply` and `world.containerSwapItemsNoCombine` that attempts to combine items before swapping and returns the leftovers if stacking was successful or the previous contents of the container slot if the items did not stack.

---

#### `Json` world.callScriptedEntity(`EntityId` entityId, `String` functionName, [`Json...` args])
#### `LuaValue` world.callScriptedEntity(`EntityId` entityId, `String` functionName, [`LuaValue...` args])

Attempts to call the specified function name in the context of the specified scripted entity with any specified arguments and returns the result of that call. This method is synchronous and thus can only be used on local master entities, i.e. scripts run on the server may only call scripted entities (on the same world) that are also server-side mastered, and scripts run on the client may only call scripted entities that are client-side mastered on that client.

On xStarbound with `"safeScripts"` enabled, all arguments must be valid JSON, and an error will be thrown after the script call if the returned result isn't convertible to valid JSON.

For more featureful entity messaging, use `world.sendEntityMessage`. To call a world script context, use `world.callScriptContext` server-side or set up an appropriate message handler in a server-side script and send an entity message that invokes it client-side.

> **Warning:** If `"safeScripts"` is disabled on xStarbound, and regardless of this on other Starbound servers and clients, potentially unsafe Lua values can be passed through `args` and/or returned through this function's return value.
>
> If unsafe passing is allowed, you should avoid passing Lua bindings or anything that can call them. Calling entity bindings after the entity has been removed from the game *will* almost certainly cause segfaults or memory corruption!

---

#### `RpcPromise<Json>` world.sendEntityMessage(`Variant<EntityId, String>` entityId, `String` messageType, [`LuaValue` args ...])

Sends an asynchronous message to an entity with the specified entity ID or unique ID with the specified message type and arguments and returns an `RpcPromise` which can be used to receive the result of the message when available. See the message table for information on entity message handling. This function **should not be called in any entity's `init` function**, as the sending entity will not have been fully loaded.

See `message.md` for information on message handlers and `RpcPromise` objects.

---

#### `RpcPromise<Vec2F>` world.findUniqueEntity(`String` uniqueId)

Attempts to find an entity on the server by unique ID and returns an `RpcPromise` that can be used to get the position of that entity if successful.

See `message.md` for information on `RpcPromise` objects.

---

#### `bool` world.loungeableOccupied(`EntityId` entityId)

Checks whether the specified loungeable entity is currently occupied and returns `true` if it is occupied, `false` if it is unoccupied, or `nil` if it is not a loungeable entity.

---

#### `bool` world.isMonster(`EntityId` entityId, [`bool` aggressive])

Returns `true` if the specified entity exists and is a monster and `false` otherwise. If aggressive is specified, will return `false` unless the monster's aggressive state matches the specified value.

---

#### `String` world.monsterType(`EntityId` entityId)

Returns the monster type of the specified monster, or `nil` if the entity is not a monster.

---

#### `bool` world.isNpc(`EntityId` entityId, [`int` damageTeam])

Returns `true` if the specified entity exists and is an NPC and `false` otherwise. If damageTeam is specified, will return `false` unless the NPC's damage team number matches the specified value.

---

#### `bool` world.isEntityInteractive(`EntityId` entityId)

Returns `true` if an entity with the specified ID is player interactive and `false` otherwise.

---

#### `String` world.npcType(`EntityId` entityId)

Returns the NPC type of the specified NPC, or `nil` if the entity is not an NPC.

---

#### `String` world.stagehandType(`EntityId` entityId)

Returns the stagehand type of the specified stagehand, or `nil` if the entity is not a stagehand.

---

#### `void` world.debugPoint(`Vec2F` position, `Color` color)

Displays a point visible in debug mode at the specified world position.

---

#### `void` world.debugLine(`Vec2F` startPosition, `Vec2F` endPosition, `Color` color)

Displayes a line visible in debug mode between the specified world positions.

---

#### `void` world.debugPoly(`PolyF` poly, `Color` color)

Displays a polygon consisting of the specified points that is visible in debug mode.

---

#### `void` world.debugText(`String` formatString, [`LuaValue` formatValues ...], `Vec2F` position, `Color` color)

Displays text visible in debug mode at the specified position using the specified format string and optional formatted values.

---

## Server-side bindings

The following `world` bindings are available only in server-side scripts.

---

#### `Json` world.callScriptContext(`String` contextName, `String` functionName, [`Json...` args])
#### `LuaValue` world.callScriptContext(`String` contextName, `String` functionName, [`LuaValue...` args])

> **Available only on xStarbound and OpenStarbound.**

Attempts to call the specified function name in the specified world script context (on the same world as the context or entity calling this binding) with any specified arguments and returns the result of that call.

On xStarbound with `"safeScripts"` enabled, all arguments must be valid JSON, and an error will be thrown after the script call if the returned result isn't convertible to valid JSON.

To message *other* worlds, use `universe.sendWorldMessage` in a world context script (see `universeserver.md`). If you need to message another world from a server-side entity, use `world.callScriptContext` to "pass through" to a `universe.sendWorldMessage` call. Both client- and server-side entities may also use `world.sendEntityMessage` for "passthrough".

> **Warning:** If `"safeScripts"` is disabled on xStarbound, and regardless of this on other Starbound servers and clients, potentially unsafe Lua values can be passed through `args` and/or returned through this function's return value.
>
> If unsafe passing is allowed, you should avoid passing Lua bindings or anything that can call them. Calling entity bindings after the entity has been removed from the game *will* almost certainly cause segfaults or memory corruption!

---

#### `bool` world.breakObject(`EntityId` entityId, `bool` smash)

Breaks the specified object and returns `true` if successful and `false` otherwise. If smash is `true` the object will not (by default) drop any items.

---

#### `bool` world.isVisibleToPlayer(`RectF` region)

Returns `true` if any part of the specified region overlaps any player's screen area and `false` otherwise.

---

#### `bool` world.loadRegion(`RectF` region)

Attempts to load all sectors overlapping the specified region and returns `true` if all sectors are fully loaded and `false` otherwise.

---

#### `bool` world.regionActive(`RectF` region)

Returns `true` if all sectors overlapping the specified region are fully loaded and `false` otherwise.

---

#### `void` world.setTileProtection(`DungeonId` dungeonId, `bool` protected)

Enables or disables tile protection for the specified dungeon ID.

---

#### `DungeonId` world.dungeonId(`Vec2F` position)

Returns the dungeon ID at the specified world position.

---

#### `DungeonId` world.setDungeonId(`RectI` tileArea, `DungeonId` dungeonId)

Sets the dungeon ID of all tiles within the specified area.

---

#### `RpcPromise<Vec2I>` world.enqueuePlacement(`List<Json>` distributionConfigs, [`DungeonId` id])

Enqueues a biome distribution config for placement through world generation. The returned promise is fulfilled with the position of the placement, once it has been placed.

See `message.md` for information on `RpcPromise` objects.

---

#### `bool` world.isPlayerModified(`RectI` region)

Returns `true` if any tile within the specified region has been modified (placed or broken) by a player and `false` otherwise.

---

#### `LiquidLevel` world.forceDestroyLiquid(`Vec2F` position)

Identical to `world.destroyLiquid`, but ignores tile protection.

**Note:** There is currently no equivalent server-side callback for forcibly destroying tiles. Instead, disable tile protection first, destroy the tiles in question with `world.damageTiles` or `world.damageTileArea`, then re-enable it.

---

#### `EntityId` world.loadUniqueEntity(`String` uniqueId)

Forces (synchronous) loading of the specified unique entity, and returns its non-unique entity ID, or `0` if no such unique entity exists.

---

#### `void` world.setUniqueId(`EntityId` entityId, [`String` uniqueId])

Sets the unique ID of the specified entity to the specified unique ID, or clears it if no unique ID is specified.

---

#### `ItemDescriptor` world.takeItemDrop(`EntityId` targetEntityId, [`EntityId` sourceEntityId])

Takes the specified item drop and returns an `ItemDescriptor` of its contents or `nil` if the operation fails. If a source entity ID is specified, the item drop will briefly animate toward that entity.

---

#### `void` world.setPlayerStart(`Vec2F` position, [`bool` respawnInWorld])

Sets the world's default beam-down point to the specified position. If respawnInWorld is set to `true` then players who die in that world will respawn at the specified start position rather than being returned to their ships.

---

#### `List<EntityId>` world.players()

Returns a list of the entity IDs of all players currently in the world.

---

#### `String` world.fidelity()

Returns the name of the fidelity level at which the world is currently running. See worldserver.config for fidelity configuration.

---

#### `String` world.flyingType()

Returns the current flight status of a ship world.

---

#### `String` world.warpPhase()

Returns the current warp phase of a ship world.

---

#### `void` world.setUniverseFlag(`String` flagName)

Sets the specified universe flag on the current universe.

---

#### `List<String>` world.universeFlags()

Returns a list of all universe flags set on the current universe.

---

#### `bool` world.universeFlagSet(`String` flagName)

Returns `true` if the specified universe flag is set and `false` otherwise.

---

#### `double` world.skyTime()

Returns the current time for the world's sky.

---

#### `void` world.setSkyTime(`double` time)

Sets the current time for the world's sky to the specified value. If `math.huge` is specified, disables the pegging of the world's clock to the universe clock *without* setting the time; if `-math.huge` is specified, re-enables the clock pegging, which will change the world's sky time to conform with the universe clock on the same tick if there's a mismatch. Clock pegging is automatically (re-)enabled whenever a world is unloaded and reloaded; i.e., it's *not* saved to the world file automatically, so find a way to save what the world's time should be if you wish to do so.

**Note:** This callback is buggy to the point of uselessness on the stock Starbound server and various other servers — any newly set time would just get reverted on the same world tick. xStarbound fixes this bug by keeping track of whether the world's clock is actually overridden and assigning special meanings to the infinity values.

---

#### `void` world.setExpiryTime(`float` expiryTime)

> **Available only on xStarbound and OpenStarbound.**

Sets the amount of time remaining on the world expiration timer. This timer starts ticking down when there are no longer any players on the world, and using this callback regularly to reset the expiry timer allows a world to persist indefinitely. To immediately cause a world to be unloaded while players aren't present, pass a value of `0` to this callback.

**Note:** If a negative float value is passed to the callback, the world will never expire. At least not until this callback is invoked again with a positive or zero value.

---

#### `void` world.placeDungeon(`String` dungeonName, `Vec2I` position, [`DungeonId` dungeonId])

Generates the specified dungeon in the world at the specified position, ignoring normal dungeon anchoring rules. If a dungeon ID is specified, it will be assigned to the dungeon.

---

#### `void` world.placeDungeon(`String` dungeonName, `Vec2I` position, [`DungeonId` dungeonId])

Generates the specified dungeon in the world at the specified position. Does not ignore anchoring rules, will fail if the dungeon can't be placed. If a dungeon ID is specified, it will be assigned to the dungeon.

---

#### `void` world.addBiomeRegion(`Vec2I` position, `String` biomeName, `String` subBlockSelector, `int` width)

Adds a biome region to the world, centered on `position`, `width` blocks wide.

---

#### `void` world.expandBiomeRegion(`Vec2I` position, `int` width)

Expands the biome region currently at `position` by `width` blocks.

---

#### `void` world.pregenerateAddBiome(`Vec2I` position, `int` width)

Signals a region for asynchronous generation. The region signaled is the region that needs to be generated to add a biome region of `width` tiles to `position`.

---

#### `void` world.pregenerateExpandBiome(`Vec2I` position, `int` width)

Signals a region for asynchronous generation. The region signaled is the region that needs to be generated to expand the biome at `position` by `width` blocks.

---

#### `void` world.setLayerEnvironmentBiome(`Vec2I` position)

Sets the environment biome for a layer to the biome at `position`.

---

#### `void` world.setPlanetType(`String` planetType, `String`, primaryBiomeName)

Sets the planet type of the current world to `planetType` with primary biome `primaryBiomeName`.

---

#### `void` world.setDungeonGravity(`DungeonId` dungeonId, `Maybe<float>` gravity)

Sets the overriding gravity for the specified dungeon ID, or returns it to the world default if unspecified.

---

#### `void` world.setDungeonBreathable(`DungeonId` dungeonId, `Maybe<bool>` breathable)

Sets the overriding breathability for the specified dungeon ID, or returns it to the world default if unspecified.

---

## Client-side bindings

The following `world` bindings are available only in client-side scripts.

---

#### `RectI` world.clientWindow

Returns the bounds of what is visible in the client window in world tiles.

---

#### `void` world.setLightMultiplier(`RgbColorMultiplier` colour)

> **Available only on xStarbound.**

Multiplies the RGB colour of all rendered light sources in the world is (client-sidedly) multiplied by the respective value in the given colour multiplier array (red multiplies red, and so on). This is reset every tick, so you should invoke it every tick.

The `colour` parameter is an array of three floats — one each for red, green and blue, respectively. Example:

```lua
world.setLightMultiplier({5.0, 2.5, 4.5})
```

The callback allows techs and status effects to grant the player "night vision" (or make the player nearly blind, as the case may be).

---

#### `void` world.setShaderParameters(`Maybe<Vec3F>` param1, `Maybe<Vec3F>` param2, `Maybe<Vec3F>` param3, `Maybe<Vec3F>` param4, `Maybe<Vec3F>` param5, `Maybe<Vec3F>` param6)

> **Available only on xStarbound.**

Sets the corresponding scriptable shader parameters to the given values. Any `nil` or unspecified parameter is left unchanged.

The parameters are used in the GLSL shader file `$assets/rendering/effects/world.frag` (see `$src/assets/xSBscripts/rendering/effects/world.frag` for a usage example). Feel free to come up with your own uses for these parameters!

> **Shader compilation:** A full client restart is required to recompile and reload shader files — `/reload` isn't enough — and the client will shut down immediately if any compilation errors come up; check `xclient.log`.

---

#### `void` world.resetShaderParameters()

> **Available only on xStarbound.**

Resets all scriptable shader parameters to `{0.0, 0.0, 0.0}` (in GLSL, `vec3(0.0, 0.0, 0.0)`).

Note that these parameters are automatically reset whenever the player is warped to a new world (including upon player death) or "warp-reset" to the same world (where the world is unloaded and then reloaded on the client).

---

#### `Vec3F, Vec3F, Vec3F, Vec3F, Vec3F, Vec3F` world.getShaderParameters()

> **Available only on xStarbound.**

Returns the current values of the scriptable shader parameters. Consider using `table.pack` on the returned values.

---

#### `List<EntityId>` world.players()

> **Available only on xStarbound and OpenStarbound.**

Returns a list of entity IDs for all player entities currently loaded/rendered by the client.

---

#### `List<EntityId>` world.ownPlayers()

> **Available only on xStarbound.**

Returns a list of entity IDs for all player entities currently mastered by the client (and loaded into the world).

---

#### `EntityId` world.primaryPlayer()

> **Available only on xStarbound.**

Returns the entity ID of the client's primary player — i.e., the one to and from which client input and output are currently being passed.

---

#### `List<Uuid>` world.ownPlayerUuids()

> **Available only on xStarbound.**

Returns a list of UUIDs for all player entities currently mastered by the client. Includes players not currently loaded in the world (e.g., currently dead players).

---

#### `List<Uuid>` world.primaryPlayerUuid()

> **Available only on xStarbound.**

Returns the UUID of the client's primary player — i.e., the one to and from which client input and output are currently being passed.

---

#### `bool` world.swapPlayer(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Attempts to swap the primary player to the player with the specified UUID, as if `/swapuuid` had been used; does not log a chat message. Returns `true` if the swap was successful, or `false` otherwise. The actual swap does not happen until the next game tick.

---

#### `bool` world.addPlayer(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Attempts to add a secondary player with the specified UUID, as if `/adduuid` had been used; does not log a chat message. Returns `true` if the swap was successful, or `false` otherwise.

---

#### `bool` world.removePlayer(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Attempts to remove a secondary player with the specified UUID, as if `/removeuuid` had been used; does not log a chat message. Returns `true` if the swap was successful, or `false` otherwise.

---

#### `JsonObject` world.ownPlayerNames()

> **Available only on xStarbound v3.1+.**

Returns a map of player UUIDs to player names in the client's saves. Only saves that have been validated during pre-loading (and thus are successfully loadable) will be listed.

---

#### `JsonObject` world.ownPlayerSaves()

> **Available only on xStarbound v3.1+.**

Returns a map of player UUIDs to player save data objects (`JsonObject`) in the client's saves. Only saves that have been validated during pre-loading (and thus are successfully loadable) will be listed.

> **Note:** This callback returns a *lot* of data and may cause a frame spike when invoked. Consider using `world.ownPlayerSave` (in the singular) instead if you only want to load a specific save.

---

#### `Maybe<JsonObject>` world.ownPlayerSave(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Returns the player save data for the player with the specified UUID, or `nil` if the player save does not exist on the client or is invalid.

> **Note:** This callback may return a *lot* of data and cause a frame spike when invoked. Try to avoid invoking it every tick.

---

#### `Maybe<bool>` world.playerDead(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Returns whether the player with the specified UUID is currently dead — `true` if dead, `false` if alive. A dead player will not respawn unless the current world allows for dead players to respawn or the dead player has `"alwaysRespawnOnWorld"` active.

Note that permadead players will only respawn if the *primary* player is an admin and any other respawning conditions are met for the permadead player as a *secondary*.

Will return `nil` if the player doesn't exist, the player save failed validation (and thus is not loadable) or the specified UUID is invalid.

---

#### `Maybe<bool>` world.playerLoaded(`Uuid` playerUuid)

> **Available only on xStarbound v3.1+.**

Returns whether the player with the specified UUID is currently loaded and active. Note that this callback will return `false` instead of throwing an error if the specified UUID is invalid.