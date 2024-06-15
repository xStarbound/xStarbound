# `effect`

The `effect` table relates to callbacks specifically for status effects and is only available in status effect scripts.

---

#### `float` effect.duration()

Returns the remaining duration of the status effect.

---

#### `void` effect.modifyDuration(`float` duration)

Adds the specified duration to the current remaining duration.

---

#### `void` effect.expire()

Immediately expire the effect, setting the duration to 0. The effect after this tick's `update` completes, causing `uninit` to be invoked.

---

#### `EntityId` effect.sourceEntity()

Returns the source entity ID for the status effect, if any.

---

#### `void` effect.setParentDirectives(`String` directives)

Sets image processing directives for the entity the status effect is active on.

---

#### `Json` effect.getParameter(`String` name, `Json` default)

Returns the value associated with the parameter name in the effect configuration. If no value is set, returns the specified default value; if the value is explicitly set to `null`, returns `nil` even if a non-`nil` default is specified.

---

#### `StatModifierGroupId` effect.addStatModifierGroup(`List<StatModifier>` modifiers)

Adds a new stat modifier group and returns the ID created for the group. Stat modifier groups will stay active until the effect expires. The returned group ID is an unsigned integer.

Stat modifiers are of the following format:

```lua
jobject{
  stat = "health",

  -- *One* of the following:
  amount = 50,
  baseMultiplier = 1.5,
  effectiveMultiplier = 1.5
}
```

---

#### `void` effect.setStatModifierGroup(`StatModifierGroupId` groupId, `List<StatModifier>` modifiers)

Replaces the list of stat modifiers in a group with the specified modifiers. The `groupId` is an unsigned integer.

---

#### `void` effect.removeStatModifierGroup(`StatModifierGroupId` groupId)

Removes the specified stat modifier group. The `groupId` is an unsigned integer.
