# `stagehand`

The `stagehand` table contains bindings specific to stagehands and is available in stagehand scripts.

---

#### `EntityId` stagehand.id()

Returns the stagehand's entity ID. Identical to `entity.id`.

---

#### `Vec2F` stagehand.position()

Returns the stagehand's position. Identical to `entity.position`.

---

#### `void` stagehand.setPosition(`Vec2F` position)

Moves the stagehand to the specified position.

---

#### `void` stagehand.die()

Destroys the stagehand.

---

#### `String` stagehand.typeName()

Returns the stagehand's type name.

---

#### `void` stagehand.setUniqueId([`String` uniqueId])

Sets the stagehand's unique entity ID, or clears it if unspecified.
