# `entity`

The `entity` table contains functions that are common among all entities. Every function refers to the entity the script context is running on.

Accessible in:

- generic player scripts (only on xStarbound)
- companion system scripts
- quest scripts
- tech scripts
- primary status scripts
- status effects
- monster scripts
- NPC scripts
- object scripts
- active item scripts

---

#### `EntityId` entity.id()

Returns the in-game ID of the entity.

---

#### `Json` entity.damageTeam()

Returns a table containing the entity's damage team type and team number, or `nil` if an invalid entity ID is specified. Example: `jobject{type = "enemy", team = 0}`.

---

#### `bool` entity.isValidTarget(`EntityId` entityId)

Returns whether the provided entity is a valid target of the current entity. An entity is a valid target if it can be damaged, and in the case of monsters and NPCs, if it is marked as aggressive.

---

#### `Vec2F` entity.distanceToEntity(`EntityId` entityId)

Returns the distance, as an `{X, Y}` vector, from the current entity to the provided entity. Use the Pythagorean theorem to obtain a linear distance.

---

#### `bool` entity.entityInSight(`EntityId` entityId)

Returns whether the provided entity is in line of sight of the current entity.

---

#### `Vec2F` entity.position()

Returns the current world position of the entity.

---

#### `String` entity.entityType()

Returns the type of the current entity. See `world.entityType` in `world.md` for a list of possible entity types.

---

#### `Maybe<String>` entity.uniqueId()

Returns the unique ID of the entity, or `nil` if there is no unique ID.

---

#### `bool` entity.persistent()

Returns `true` if the entity is persistent (will be saved to disk on sector unload) or `false` otherwise.
