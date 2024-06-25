# `fireableItem`

The `fireableItem` table is available in scripts for "fireable" items. The following types of items are considered "fireable" and can (indeed) run scripts:

- liquid items
- material items
- placeable object items
- thrown items
- tools (including beamaxes)
- consumables

Active items are *not* considered "fireable", but have the `activeItem` table instead (see `activeitem.md`).

**Note:** Fireable item scripts and active item scripts taken together basically cover every held item that a player or NPC can "fire" in some sense, except blueprint items.

**Note 2:** Contrary to the [official Starbound wiki](https://starbounder.org/Modding:Lua/Tables/FireableItem), `activeItem.setFireableParam` doesn't actually exist. Not that this matters, since `item.setInstanceValue` *does* exist.

----

#### `void` fireableItem.fire(`String` mode)

Triggers the item to fire.

- For thrown weapons, this means throwing or firing the weapon (`"primary"`).
- For liquid, material and object items, this means attempting to place the object, liquid or material. Materials are placed in the foreground (`"primary"`) or background (`"alt"`).
- For beamaxes (matter manipulators) and other mining tools, this means attempting to mine the foreground (`"primary"`) or background (`"alt"`)
- For wiring tools, this means attempting to connect a wire node (`"primary"`) or remove a wire node (`"alt"`)
- For paint tools, this means attempting to paint tiles (`"primary"`) or switch the paint colour (`"alt"`).
- For consumables, this means attempting to consume the item (`"primary"`).

**Tip:** To set a fireable item's aim position, invoke `player.controlAimPosition(aimPosition)` for a player, or `world.callScriptedEntity(entity.id(), "npc.setAimPosition", aimPosition)` for an NPC.

----

#### `void` fireableItem.triggerCooldown()

Triggers the item's cooldown.

----

#### `void` fireableItem.setCooldown(`float` cooldownTime)

Sets the item's cooldown time.

**Note:** Contrary to the [official Starbound wiki](https://starbounder.org/Modding:Lua/Tables/FireableItem), this callback now works correctly and sets only the current item's cooldown time in Starbound 1.4.4.

----

#### `void` fireableItem.endCooldown()

Ends the item's cooldown.

----

#### `float` fireableItem.cooldownTime()

Returns the item's current cooldown time.

----

#### `Json` fireableItem.fireableParam(`String` key, `Json` default)

Returns the value of the specified parameter, or `default` if the parameter doesn't exist. Explicitly `null` parameters count as "existing" for this callback.

**Note:** This callback cannot take JSON subpaths (see `root.md`) and thus can only access root-level keys. Also see `config.getParameter`, which *can* take JSON subpaths (see `config.md`).

----

#### `String` fireableItem.fireMode()

Returns the current fire mode of this item — `"Primary"`, `"Alt"` or `"None"` (not firing).

----

#### `bool` fireableItem.ready()

Returns whether the item is currently ready to be fired.

----

#### `bool` fireableItem.firing()

Returns whether the item is currently firing.

----

#### `bool` fireableItem.windingUp()

Returns whether the item is currently winding up to fire.

----

#### `bool` fireableItem.coolingDown()

Returns whether the item is currently cooling down after firing.

----

#### `bool` fireableItem.ownerFullEnergy()

Returns whether the wielder has a full energy stat.

----

#### `float` fireableItem.ownerEnergy()

Returns the wielder's current energy stat.

> **Note:** On stock Starbound and OpenStarbound, this callback is buggy and returns a `bool` instead — `true` if the owner's energy is above zero or `false` otherwise. xStarbound fixes this bug at the cost of potential mod script incompatibility, since `0.0` is "truthy" in Lua.
> 
> Not that anybody ever used this broken callback since it was marked as broken [on the Starbound wiki](https://starbounder.org/Modding:Lua/Tables/FireableItem).

----

#### `bool` fireableItem.ownerConsumeEnergy(`float` energyToConsume)

Attempts to consume the specified amount of the wielder's energy. This will "overconsume" energy — as long as the wielder has more than zero energy and the wielder's energy stat isn't locked, energy will be consumed, even if the wielder had less energy than requested.

As such, this callback returns `true` if the owner's energy stat is non-zero *and* not locked, or `false` if it's zero *or* locked.

Equivalent to `status.overConsumeResource("energy", energyToConsume)` (see `statuscontroller.md`), but fireable item scripts do not normally have access to the `status` table.