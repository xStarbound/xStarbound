# npc

The `npc` table is for functions relating directly to the current NPC. It is available only in NPC scripts.

---

#### `Vec2F` npc.toAbsolutePosition(`Vec2F` offset)

Returns the specified local position in world space.

---

#### `String` npc.species()

Returns the species of the npc.

---

#### `String` npc.gender()

Returns the gender of the NPC.

---

#### `Json` npc.humanoidIdentity()

Returns the NPC's humanoid identity.

---

#### `String` npc.npcType()

Returns the NPC type of the NPC.

---

#### `uint64_t` npc.seed()

Returns the seed used to generate this NPC.

---

#### `float` npc.level()

Returns the level of the NPC.

---

#### `List<String>` npc.dropPools()

Returns the list of treasure pools that will spawn when the NPC dies.

---

#### `void` npc.setDropPools(`List<String>` pools)

Sets the list of treasure pools that will spawn when the NPC dies.

---

#### `float` npc.energy()

Returns the current energy of the NPC. Equivalent to `status.resource("energy")`.

---

#### `float` npc.maxEnergy()

Returns the maximum energy of the NPC. Equivalent: `status.maxResource("energy")`.

---

#### `bool` npc.say(`String` line, [`Map<String,String>` tags], [`Json` config])

Makes the npc say a string. Optionally pass in tags to replace text tags. Optionally give config options for the chat message.

Returns whether the chat message was successfully added.

An example showing the available options:
```lua
jobject{
  drawBorder = true,
  fontSize = 8,
  color = jarray{255, 255, 255},
  sound = "/sfx/humanoid/avian_chatter_male1.ogg"
}
```

---

#### `bool` npc.sayPortrait(`String` line, `String` portrait, [`Map<String,String>` tags], [`Json` config])

Makes the npc say a line, with a portrait chat bubble. Optionally pass in tags to replace text tags. Optionally give config options for the chat message.
Returns whether the chat message was successfully added.

Available options:
```
{
  drawMoreIndicator = true,
  sound = "/sfx/humanoid/avian_chatter_male1.ogg"
}
```

---

#### `void` npc.emote(`String` emote)

Makes the NPC show a facial emote.

---

#### `void` npc.dance(`String` dance)

Sets the current dance for the NPC. Dances are defined in `.dance` files.

---

#### `void` npc.setInteractive(`bool` interactive)

Sets whether the NPC should be interactive.

---

#### `bool` npc.setLounging(`EntityId` loungeable, [`size_t` anchorIndex])

Sets the NPC to lounge in a loungeable entity. Optionally specify which anchor (seat) to use.
Returns whether the NPC successfully lounged.

---

#### `void` npc.resetLounging()

Makes the NPC stop lounging.

---

#### `bool` npc.isLounging()

Returns whether the NPC is currently lounging.

---

#### `Maybe<EntityId>` npc.loungingIn()

Returns the entity ID of the loungeable the NPC is currently lounging in. Returns `nil` if the NPC is not lounging.

---

#### `void` npc.setOfferedQuests(`JsonArray` quests)

Sets the list of quests the NPC will offer.

---

#### `void` npc.setTurnInQuests(`JsonArray` quests)

Sets the list of quests the player can turn in at this NPC.

---

#### `bool` npc.setItemSlot(`ItemSlot` slot, `ItemDescriptor` item)

Sets the specified item slot to contain the specified item.

The valid values for `slot` are:

- `"head"`
- `"headCosmetic"`
- `"chest"`
- `"chestCosmetic"`
- `"legs"`
- `"legsCosmetic"`
- `"back"`
- `"backCosmetic"`
- `"primary"`
- `"alt"`

---

#### `ItemDescriptor` npc.getItemSlot(`ItemSlot` slot)

Returns the item currently in the specified item slot. See above for valid values for `slot`.

---

#### `void` npc.disableWornArmor(`bool` disable)

Set whether the NPC should not gain status effects from the equipped armour. Armour will still be visually equipped.

---

#### `void` npc.beginPrimaryFire()

Makes the NPC start firing the item equipped in the `"primary"` item slot.

---

#### `void` npc.beginAltFire()

Makes the NPC start firing the item equipped in the `"alt"` item slot.

---

#### `void` npc.endPrimaryFire()

Stops the NPC from firing the item equipped in the `"primary"` item slot.

---

#### `void` npc.endAltFire()

Stops the NPC from firing the item equipped in the `"alt"` item slot.

---

#### `void` npc.setShifting(`bool` shifting)

Sets whether tools should be used as though the **Run** key (**Shift** by default) were held.

---

#### `void` npc.setDamageOnTouch(`bool` enabled)

Sets whether damage on touch should be enabled.

---

#### `Vec2F` npc.aimPosition()

Returns the NPC's current aim position in world coordinates.

---

#### `void` npc.setAimPosition(`Vec2F` position)

Sets the NPC's aim position in world coordinates.

---

#### `void` npc.setDeathParticleBurst(`String` emitter)

Sets a particle emitter to burst when the NPC dies.

---

#### `void` npc.setStatusText(`String` status)

Sets the text to appear above the NPC when it first appears on screen.

---

#### `void` npc.setDisplayNametag(`bool` display)

Sets whether the name tag should be displayed above the NPC.

---

#### `void` npc.setPersistent(`bool` persistent)

Sets whether this NPC should persist (i.e., be saved to the world) after being unloaded.

---

#### `void` npc.setKeepAlive(`bool` keepAlive)

Sets whether to keep this NPC alive. If true, the NPC will never be unloaded as long as the world is loaded. See also `npc.setPersistent`.

---

#### `void` npc.setDamageTeam(`Json` damageTeam)

Sets a damage team for the NPC.

Example of the required format:

```lua
jarray{ type = "enemy", team = 2 }
```

---

#### `void` npc.setAggressive(`bool` aggressive)

Sets whether the NPC should be flagged as aggressive.

---

#### `void` npc.setUniqueId(`String` uniqueId)

Sets a unique ID for this NPC that can be used to access it. If `nil` is specified, clears any existing unique ID. A unique ID has to be unique for the world the NPC is on, but not universally unique.

--- 

#### `void` npc.setIdentity(`Json` identity)

Sets the NPC's humanoid identity. The new identity will be merged with the current one; as a special case, if the `"imagePath"` key (and *only* that key) has been explicitly set to a `nil` value, *and* either the table was created with `jobject` or its metatable's `"__nils"` table otherwise contains `"imagePath"` (e.g., the metatable is `{["__nils"] = {imagePath = 0}}`), the `"imagePath"` will be set to `null`. Will log an error and leave the species unchanged if the new identity includes a `"species"` that doesn't exist in the loaded assets.

---

#### `Json` npc.parameters()

Returns the NPC's full parameters.

---

#### `void` npc.setOverrideState(`Maybe<String>` newState)

Overrides the NPC's humanoid animation state. Available states are:

- `"idle"`
- `"jump"`
- `"fall"`
- `"sit"`
- `"lay"`
- `"duck"`
- `"walk"`
- `"run"`
- `"swim"`
- `"swimIdle"`

Any other string will set the state to `"idle"`. If `nil` or no argument is passed, any existing override is cleared. Most tech parent state equivalences are obvious, but note that `"idle"` is equivalent to the tech parent state `"Stand"`, and `"jump"` is equivalent to the tech parent state `"Fly"`.