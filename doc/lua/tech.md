# `tech`

The `tech` table contains callbacks exclusively available in tech scripts.

---

#### `Vec2F` tech.aimPosition()

Returns the current cursor aim position as a world coordinate.

---

#### `void` tech.setVisible(`bool` visible)

Sets whether the tech should be visible.

---

#### `void` tech.setParentState(`String` state)

> _The `"SwimIdle"` state is available only on xStarbound and OpenStarbound. When networked to vanilla clients, the `"SwimIdle"` state looks as if **no** parent state override is applied._

Set the animation state of the player.

Valid states:

- `"Stand"`
- `"Fly"`
- `"Fall"`
- `"Sit"`
- `"Lay"`
- `"Duck"`
- `"Walk"`
- `"Run"`
- `"Swim"`
- `"SwimIdle"` (xStarbound and OpenStarbound only)

---

#### `void` tech.setParentDirectives([`Directives` directives])

Sets the image processing directives for the player. If no directives are specified, clears any existing tech parent directives. Each tech slot has its own parent directives.

---

#### `void` tech.setParentHidden(`bool` hidden)

Sets whether to make the player invisible. Will still show the tech.

---

#### `void` tech.setParentOffset(`Vec2F` offset)

Sets the position of the player relative to the tech, in world tiles.

---

#### `bool` tech.parentLounging()

Returns whether the player is lounging.

---

#### `void` tech.setToolUsageSuppressed(`bool` suppressed)

Sets whether the player can use held items. Identical to `player.setToolUsageSuppressed` (see `player.md`).

**Note:** The internal values used by _both_ `player.setToolUsageSuppressed` and `tech.setToolUsageSuppressed` must be `false` for the player to use held items.

