# `status`

The `status` table relates to the status controller attached to an entity. It is available in the following script contexts:

- monster scripts
- NPC scripts
- status effect scripts
- player companion scripts
- quest scripts
- tech scripts
- primary status scripts for players, monsters and NPCs

---

#### `Json` status.statusProperty(`String` name, `Json` default)

Returns the value assigned to the specified status property. If there is no value set, returns default.

---

#### `void` status.setStatusProperty(`String` name, `Json` value)

Sets a status property to the specified value.

---

#### `float` status.stat(`String` statName)

Returns the value for the specified stat. Defaults to `0.0` if the stat does not exist.

---

#### `bool` status.statPositive(`String` statName)

Returns whether the stat value is greater than 0.

---

#### `List<String>` status.resourceNames()

Returns a list of the names of all configured resources.

---

#### `bool` status.isResource(`String` resourceName)

Returns whether the specified resource exists in this status controller.

---

#### `float` status.resource(`String` resourceName)

Returns the value of the specified resource.

---

#### `bool` status.resourcePositive(`String` resourceName)

Returns whether the value of the specified resource is greater than zero.

---

#### `void` status.setResource(`String` resourceName, `float` value)

Sets a resource to the specified value.

---

#### `void` status.modifyResource(`String` resourceName, `float` value)

Adds the specified value to a resource.

---

#### `float` status.giveResource(`String` resourceName, `float` value)

Adds the specified value to a resource. Returns any overflow.

---

#### `bool` status.consumeResource(`String` resourceName, `float` amount)

Tries to consume the specified amount from a resource. Returns whether the full amount was able to be consumes. Does not modify the resource if unable to consume the full amount.

---

#### `bool` status.overConsumeResource(`String` resourceName, `float` amount)

Tries to consume the specified amount from a resource. If unable to consume the full amount, will consume all the remaining amount. Returns whether it was able to consume any at all of the resource.

---

#### `bool` status.resourceLocked(`String` resourceName)

Returns whether the resource is currently locked.

---

#### `void` status.setResourceLocked(`String` resourceName, `bool` locked)

Sets a resource to be locked/unlocked. A locked resource cannot be consumed.

---

#### `void` status.resetResource(`String` resourceName)

Resets a resource to its base value.

---

#### `void` status.resetAllResources()

Resets all resources to their base values.

---

#### `float` status.resourceMax(`String` resourceName)

Returns the maximum value for the specified resource.

---

#### `float` status.resourcePercentage(`String` resourceName)

Returns the percentage of the maximum value that the resource is currently at, as a fractional percentage between 0.0 and 1.0.

---

#### `void` status.setResourcePercentage(`String` resourceName, `float` value)

Sets a resource to a percentage of the maximum value for the resource, as a fractional percentage between 0.0 and 1.0.

---

#### `void` status.modifyResourcePercentage(`String` resourceName, `float` value)

Adds a percentage of the maximum resource value to the current value of the resource.

---

#### `JsonArray` status.getPersistentEffects(`String` effectCategory)

Returns a list of the currently active persistent effects in the specified effect category.

---

#### `void` status.addPersistentEffect(`String` effectCategory, `Json` effect)

Adds a status effect to the specified effect category.

---

#### `void` status.addPersistentEffects(`String` effectCategory, `JsonArray` effects)

Adds a list of effects to the specified effect category.

---

#### `void` status.setPersistentEffects(`String` effectCategory, `JsonArray` effects)

Sets the list of effects of the specified effect category. Replaces the current list active effects.

---

#### `void` status.clearPersistentEffects(`String` effectCategory)

Clears any status effects from the specified effect category.

---

#### `void` status.clearAllPersistentEffects()

Clears all persistent status effects from all effect categories.

---

#### `void` status.addEphemeralEffect(`String` effectName, [`float` duration], [`EntityId` sourceEntity])

Adds the specified unique status effect, optionally with a custom duration, and optionally with a source entity ID accessible in the status effect's script.

---

#### `void` status.addEphemeralEffects(`JsonArray` effects, [`EntityId` sourceEntity])

Adds a list of unique status effects, optionally with a source entity ID.

Unique status effects can be specified either as a string or as a JSON object. Example:

```lua
local effectList = jarray{
    "myuniqueeffect",
    jobject{
        effect = "myothereffect",
        duration = 5 -- In seconds.
    }
}
status.addEphemeralEffects(effectList)
```

---

#### `void` status.removeEphemeralEffect(`String` effectName)

Removes the specified unique status effect.

---

#### `void` status.clearEphemeralEffects()

Clears all ephemeral status effects.

---

#### `List<pair<DamageNotification>>, unsigned` status.damageTakenSince([`unsigned` since = 0])

Returns two values:

- A list of damage notifications for the entity's damage taken since the specified heartbeat.
- The most recent heartbeat value, intended to be passed into this callback again to get the damage notifications taken since the last call.

Example:

```lua
_, lastStep = status.damageTakenSince() -- Returns the full buffer of damage notifications, throw this away, we only want the current step.

-- ...

notifications,lastStep = status.damageTakenSince(lastStep) -- Get the damage notifications since the last call, and update the heartbeat.
```

---

#### `List<pair<EntityId, DamageRequest>>, unsigned` status.inflictedHitsSince([`unsigned` since = 0])

Returns two values:

- A list (e.g., `{ {entityId, damageRequest}, ...}`) for hits inflicted by the entity.
- The most recent heartbeat value, intended to be passed into the function again to get a list of inflicted hits since the last call.

---

#### `List<DamageNotification>, unsigned` status.inflictedDamageSince([`unsigned` since = 0])

Returns two values:

- A list of damage notifications for damage inflicted *by* the entity.
- The most recent heartbeat value, intended to be passed into the function again to get a list of damage notifications since the last call.

---

#### `JsonArray` status.activeUniqueStatusEffectSummary()

Returns a list of two element tables describing all unique status effects currently active on the status controller. Each entry consists of the `String` name of the effect and a `float` between 0 and 1 indicating the remaining portion of that effect's duration.

---

#### `bool` status.uniqueStatusEffectActive(`String` effectName)

Returns `true` if the specified unique status effect is currently active and `false` otherwise.

---

#### `String` status.primaryDirectives()

Returns the primary set of image processing directives applied to the animation of the entity using this status controller.

---

#### `void` status.setPrimaryDirectives([`String` directives])

Sets the primary set of image processing directives that should be applied to the animation of the entity using this status controller. If no directives are specified, any existing primary directives are cleared.

---

#### `void` status.applySelfDamageRequest(`DamageRequest` damageRequest)

Directly applies the specified damage request to the entity using this status controller.
