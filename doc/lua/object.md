# `object`

The `object` table contains bindings specific to objects which are available in server-side object scripts.

**Note:** Wiring node indexes start at `0`, not `1`.

---

#### `String` object.name()

Returns the object's type name.

---

#### `int` object.direction()

Returns the object's facing direction. This will be `1` for right or `-1` for left.

---

#### `Vec2F` object.position()

Returns the object's tile position. Identical to `entity.position()`.

---

#### `void` object.setInteractive(`bool` interactive)

Sets whether the object is currently interactive.

---

#### `String` object.uniqueId()

Returns the object's unique entity ID, or `nil` if no unique ID is set. Identical to `entity.uniqueId()`.

---

#### `void` object.setUniqueId([`String` uniqueId])

Sets the object's unique entity ID, or clears it if unspecified.

**Note:** A world error will be thrown, causing the world to immediately shut down, if the specified unique ID is already used by any other object in the world. If done on `init`, the offending object will not be there when the world is loaded again.

---

#### `RectF` object.boundBox()

Returns the object's metaBoundBox in world space.

---

#### `List<Vec2I>` object.spaces()

Returns a list of the tile spaces that the object occupies.

---

#### `void` object.setProcessingDirectives(`String` directives)

Sets the image processing directives that should be applied to the object's animation.

---

#### `void` object.setSoundEffectEnabled(`bool` enabled)

Enables or disables the object's persistent sound effect, if one is configured.

---

#### `void` object.smash([`bool` smash])

Breaks the object. If smash is `true` then it will be smashed, causing it to (by default) drop no items.

**Note:** Contrary to popular rumours in the modding community, there is no `object["break"]`. And yes, if it did exist, you would have to call it that way to avoid a collision with the Lua keyword.

---

#### `float` object.level()

Returns the `"level"` parameter if set, otherwise returns the current world's threat level.

---

#### `Vec2F` object.toAbsolutePosition(`Vec2F` relativePosition)

Returns an absolute world position calculated from the given relative position.

---

#### `bool` object.say(`String` line, [`Map<String, String>` tags], [`Json` config])

Spawns a chat bubble over the object containing the specified `line` of text, optionally replacing any specified tags in the text, and using the provided additional chat configuration. Returns `true` if any chat bubble is spawned (i.e. the line is not empty) and `false` otherwise.

An example showing the available options in `config`:
```lua
jobject{
  drawBorder = true, -- Whether to draw the chat bubble behind the text. Sort of visually borked right now.
  fontSize = 8, -- Obvious.
  color = jarray{255, 255, 255}, -- The base colour of the text, before any escape codes are applied.
  sound = "/sfx/humanoid/avian_chatter_male1.ogg" -- A sound to play when the chat bubble spawns.
}
```

---

#### `bool` object.sayPortrait(`String` line, `String` portrait, [`Map<String, String>` tags], [`Json` config])

Similar to `object.say`, but uses a portrait chat bubble with the specified portrait image.

An example showing the available options in `config`:
```lua
{
  drawMoreIndicator = true, -- Draw an indicator that shows there's more messages coming.
  sound = "/sfx/humanoid/avian_chatter_male1.ogg" -- A sound to play when the chat bubble spawns.
}
```

---

#### `bool` object.isTouching(`EntityId` entityId)

Returns `true` if the specified entity's collision area overlaps the object's bound box and `false` otherwise.

---

#### `void` object.setLightColor(`Color` color)

Sets the color of light for the object to emit. This is not the same as animator.setLightColor and the animator light configuration should be used for more featureful light sources.

---

#### `Color` object.getLightColor()

Returns the object's currently configured light color.

---

#### `unsigned` object.inputNodeCount()

Returns the number of wire input nodes the object has.

---

#### `unsigned` object.outputNodeCount()

Returns the number of wire output nodes the object has.

---

#### `Vec2I` object.getInputNodePosition(`unsigned` nodeIndex)

Returns the relative position of the specified wire input node. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `Vec2I` object.getOutputNodePosition(`unsigned` nodeIndex)

Returns the relative position of the specified wire output node. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `bool` object.getInputNodeLevel(`unsigned` nodeIndex)

Returns the current level of the specified wire input node. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `bool` object.getOutputNodeLevel(`unsigned` nodeIndex)

Returns the current level of the specified wire output node. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `bool` object.isInputNodeConnected(`unsigned` nodeIndex)

Returns `true` if any wires are currently connected to the specified wire input node and `false` otherwise. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `bool` object.isOutputNodeConnected(`unsigned` nodeIndex)

Returns `true` if any wires are currently connected to the specified wire output node and `false` otherwise. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `Map<EntityId, unsigned>` object.getInputNodeIds(`unsigned` nodeIndex)

Returns a map of the entity ID of each wire entity connected to the given wire input node and the index of that entity's output node to which the input node is connected. Will return any connections to invalid output nodes.

---

#### `Map<EntityId, unsigned>` object.getOutputNodeIds(`unsigned` nodeIndex)

Returns a map of the entity ID of each wire entity connected to the given wire output node and the index of that entity's input node to which the output node is connected. Will return any connections to invalid input nodes.

---

#### `void` object.setOutputNodeLevel(`unsigned` nodeIndex, `bool` level)

Sets the level of the specified wire output node. Will throw an error if an invalid (out-of-range) node is specified.

---

#### `void` object.setAllOutputNodes(`bool` level)

Sets the level of all wire output nodes.

---

#### `void` object.setOfferedQuests([`JsonArray` quests])

Sets the list of quests that the object will offer to start, or clears them if unspecified.

---

#### `void` object.setTurnInQuests([`JsonArray` quests])

Sets the list of quests that the object will accept turn-in for, or clears them if unspecified.

---

#### `void` object.setConfigParameter(`String` key, `Json` value)

Sets the specified overridden configuration parameter for the object.

---

#### `void` object.setAnimationParameter(`String` key, `Json` value)

Sets the specified animation parameter for the object's scripted animator.

---

#### `void` object.setMaterialSpaces([`JsonArray` spaces])

Sets the object's material spaces to the specified list, or clears them if unspecified. List entries should be in the form of `pair<Vec2I, String>` specifying the relative position and material name of materials to be set. 

**Warning:** Objects should only set material spaces within their occupied tile spaces to prevent Bad Things™ (i.e., weird collision and tile placement) from happening.

---

#### `void` object.setDamageSources([`Maybe<List<DamageSource>>` damageSources])

Sets the object's active damage sources (or clears them if unspecified).

---

#### `float` object.health()

Returns the object's current health.

---

#### `void` object.setHealth(`float` health)

Sets the object's current health.
