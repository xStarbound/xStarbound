# `vehicle`

The `vehicle` table contains bindings specific to vehicles which are available only in vehicle scripts.

**xStarbound note:** On xStarbound v4.5.2.8+, nonexistent lounge positions, damage sources, force regions and moving collisions are ignored by these bindings instead of throwing errors.

---

#### `bool` vehicle.controlHeld(`String` loungeName, `String` controlName)

Returns `true` if the specified control is currently being held by a player occupant of the specified lounge position, or `false` otherwise (e.g., an invalid control name is specified, the player is not holding the control, the occupant is not a player character or there is no occupant at that spot). Valid control names are:

- `"Left"`
- `"Right"`
- `"Down"`
- `"Up"`,
- `"Jump"`
- `"PrimaryFire"`
- `"AltFire"`
- `"Special1"`
- `"Special2"`
- `"Special3"`
- `"Walk"` (requires xStarbound v4.5.2.8+, OpenStarbound or oSBM on vehicle master)

> **Note:** The `"Walk"` parameter, which is `true` when the **<kbd>Walk</kbd>** keybind (**<kbd>Left Shift</kbd>** by default) is held by the occupant in question, is supported in scripts as long as the vehicle's _master_ (whether an owning client or the server) supports it. No client-side support is necessary for vehicle riders aside from the master client or server.

---

#### `bool` vehicle.shiftingHeld(`String` loungeName)

> **This binding is only available on xStarbound v4.5.2.8+, OpenStarbound and oSBM.**

Returns whether any player occupying the specified lounge position is holding the **<kbd>Walk</kbd>** keybind (**<kbd>Left Shift</kbd>** by default), or `false` if there is no player character lounging there. Identical to `vehicle.controlHeld(loungeName, "Walk")`.

---

#### `Maybe<Vec2F>` vehicle.aimPosition(`String` loungeName)

Returns the world aim position for the specified lounge position.

> On xStarbound, this returns `nil` instead of throwing if the lounge position doesn't exist.

---

#### `EntityId` vehicle.entityLoungingIn(`String` loungeName)

Returns the entity ID of the entity currently occupying the specified lounge position, or `nil` if the lounge position is unoccupied.

> On xStarbound, this returns `nil` instead of throwing if the lounge position doesn't exist.

---

#### `void` vehicle.setLoungeEnabled(`String` loungeName, `bool` enabled)

Enables or disables the specified lounge position.

---

#### `void` vehicle.setLoungeOrientation(`String` loungeName, `String` orientation)

Sets the lounge orientation for the specified lounge position. Valid orientations are `"sit"`, `"stand"` or `"lay"`.

---

#### `void` vehicle.setLoungeEmote(`String` loungeName, [`String` emote])

Sets the emote to be performed by entities occupying the specified lounge position, or clears it if no emote is specified.

---

#### `void` vehicle.setLoungeDance(`String` loungeName, [`String` dance])

Sets the dance to be performed by entities occupying the specified lounge position, or clears it if no dance is specified.

---

#### `void` vehicle.setLoungeStatusEffects(`String` loungeName, `JsonArray` statusEffects)

Sets the list of status effects to be applied to entities occupying the specified lounge position. To clear the effects, set an empty list.

---

#### `void` vehicle.setPersistent(`bool` persistent)

Sets whether the vehicle is persistent, i.e., whether it will be stored when the world is unloaded and reloaded.

---

#### `void` vehicle.setInteractive(`bool` interactive)

Sets whether the vehicle is currently interactive.

---

#### `void` vehicle.setDamageTeam(`DamageTeam` team)

Sets the vehicle's current damage team type and number.

---

#### `void` vehicle.setMovingCollisionEnabled(`String` collisionName, `bool` enabled)

Enables or disables the specified collision region.

---

#### `void` vehicle.setForceRegionEnabled(`String` regionName, `bool` enabled)

Enables or disables the specified force region.

---

#### `void` vehicle.setDamageSourceEnabled(`String` damageSourceName, `bool` enabled)

Enables or disables the specified damage source.

---

#### `void` vehicle.destroy()

Destroys the vehicle.
