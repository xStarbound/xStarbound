# `message` and `RpcPromise` methods

> *World messages are available only on xStarbound and OpenStarbound. Universe client and pane message handlers are available only on xStarbound v3.4.5+.*

The `message` table contains a single callback, `setHandler`, which allows entities to receive messages sent using `world.sendEntityMessage` or `universe.sendWorldMessage`. Entities which can receive messages include:

- monsters
- NPCs
- objects
- vehicles
- stagehands
- projectiles

Additionally, messages can be handled by a variety of script contexts that run on players:

- active item scripts
- fireable item scripts
- quest scripts
- player companion scripts
- player generic scripts
- status scripts

Additionally, on xClient, messages can be handled by the following contexts:

- universe client scripts
- pane scripts with access to `world` (i.e., *not* run in the game's main menu)
- container pane scripts

Sending a message to any player controlled by an xClient client also sends that message to the client's universe client scripts, scripted panes and container pane scripts.

Lastly, world server scripts can receive (on xServer and OpenStarbound servers) world messages and (on xServer v3.5.1+) entity messages with a specified unique entity ID of `"server"` [^1] from any entity on the same world (regardless of whether the entity is server- or client-mastered). For communication between a world server script and server-side scripted entities on that world, you can also use `world.callScriptedEntity` for world-to-entity communication and `world.callScriptContext` for entity-to-world communication; this is your only option on an OpenStarbound server.

[^1]: xServer v3.5.1+ reserves the unique entity ID `"server"` specifically for receiving world server messages from entities.

---

#### `void` message.setHandler(`Variant<String, JsonObject>` message, `LuaFunction` handler)

> *On stock Starbound, `message` can only be a `String`, making the extra JSON options not available.*

Once invoked, messages of the specified message type received by this script context will call the specified function. These messages can be networked.

`message` may be either a string or a JSON message specifier, which has the following format:

```lua
jobject{
    passName = true, -- Whether to include the message name as an argument to the handler.
    localOnly = true, -- Whether this message is to be networked or to remain local.
    name = "myMessage" -- The message name.
}
```

If `message` is a string, `passName` is assumed to be `true` and `localOnly` is assumed to be `false`.

The handler function must have an appropriate signature for the values of `passName` and `localOnly`. The signatures are:

|                         | **`passName = true`**                                             | **`passName = false`**                             |
| ----------------------- | ----------------------------------------------------------------- | -------------------------------------------------- |
| **`localOnly = true`**  | `Json` handlerFunc(`String` name, `Json...` args)                 | `Json` handlerFunc(`Json...` args)                 |
| **`localOnly = false`** | `Json` handlerFunc(`String` name, `bool` isLocal, `Json...` args) | `Json` handlerFunc(`bool` isLocal, `Json...` args) |

Note that `isLocal` is always `true` for world messages — all worlds run on the same server.

The `handler` may optionally return a JSON-encodable value which can be accessed via the `RpcPromise:result` method if the response to the message is successfully received.

**Note:** Local messages always immediately succeed; you can just call `:result` on them as soon as you receive them.

---

## `RpcPromise<T>` methods

`RpcPromise<T>` objects always have a specific subtype specified in angle brackets, which is the type of the result. `RpcPromise<T>` objects have the following methods:

#### `bool` `RpcPromise<T>`:finished()

Returns whether this `RpcPromise` has been received on the other end, regardless of success or failure. For locally received messages on the same world, this is always `true`.

#### `bool` `RpcPromise<T>`:succeeded()

Returns whether this `RpcPromise` has been successfully received and handled by a handler on the other end.

If `:finished` returns `true` but `:succeeded` returns `false`, this `RpcPromise` has been received, but an error has occurred on the other end. The most common errors are missing (or misspelt) handlers and nonexistent entities.

#### `Maybe<T>` `RpcPromise<T>`:result()

Returns any result returned by the handler, or if the message hasn't been received by a handler, `nil`. For message handlers set by `message.setHandler`, `T` is `Json` — but note that `Json` includes `nil`, so message handlers don't have to return anything.

#### `Maybe<String>` `RpcPromise<T>`:error()

Returns `nil` if this `RpcPromise` has succeeded or not yet finished, or a string explaining the error if it has failed for any reason.

---

## Hardcoded entity and world messages

The following entity and world message types are sent and/or received by the engine itself, not Lua scripts:

---

#### `void` `"queueRadioMessage"` [player] (`Json` config, [`float` delay])

Queues the given radio message for the player. If a delay is specified, the radio message pop-up will be delayed by that many seconds. Received and handled by the client's engine.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalRadioMessagesIgnored`; this does not mark the message as failed.

---

#### `void` `"interruptRadioMessage"` [player] ()

Immediately interrupts any radio message currently being displayed for the primary player. Received and handled by the client's engine.

If invoked on a secondary player, the interruption will be "queued" until the 

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalRadioMessagesIgnored`; this does not mark the message as failed.

---

#### `void` `"warp"` [player] (`String` location, [`float` delay])

Immediately warps the player to the given location, specified as a world ID.

Only warps the *primary* player immediately. Any attempt to warp a secondary player will be "queued" until that player becomes primary or cleared upon disconnection. Received and handled by the client's engine.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalWarpsIgnored`; this does not mark the message as failed.

---

#### `void` `"playCinematic"` [player] (`Json` cinematic, `bool` unique)

Triggers the specified cinematic to be displayed for the player. If `unique` is `true`, the cinematic will only be shown to that player once. Received and handled by the client's engine.

If invoked on a secondary player, the cinematic is "queued" until that player becomes primary or cleared upon disconnection.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalCinematicsIgnored`; this does not mark the message as failed.

---

#### `void` `"playAltMusic"` [player] (`StringList` tracks, [`float` fadeTime])

Triggers the specified alternate music tracks to be played for the player, optionally with the specified fade-in time in seconds. Received and handled by the client's engine.

If invoked on a secondary player, the music tracks are "queued" until that player becomes primary or cleared upon disconnection.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalCinematicsIgnored`; this does not mark the message as failed.

---

#### `void` `"stopAltMusic"` [player] ([`float` fadeTime])

Stops alternate music tracks from being played for the player, optionally with the specified fade-in time in seconds. Received and handled by the client's engine.

If invoked on a secondary player, the stoppage is "queued" until that player becomes primary or cleared upon disconnection.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalCinematicsIgnored`; this does not mark the message as failed.

---

#### `void` `"recordEvent"` [player] (`String` name, `Json` fields)

Will immediately call any appropriate configured event script for the player, passing the event name and arbitrary JSON to the script's `event` function (see `statistics.md`). Received and handled by the client's engine.

The engine also sends this message locally to the consuming player whenever the player consumes any item, with the arguments `"useItem"` and `jobject{itemType = itemName}`, where `itemName` is the item's internal name.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalRadioMessagesIgnored`; this does not mark the message as failed.

---

#### `void` `"addCollectable"` [player] (`String` collection, `String` collectable)

Immediately marks the specified collectable in the specified collection as collected. Received and handled by the client's engine.

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalRadioMessagesIgnored`; this does not mark the message as failed.

---

#### `void` `"chatMessage"` [player] (`Json` message)
#### `void` `"newChatMessage"` [player] (`Json` message)

**`"chatMessage"` is available only on xStarbound. `"newChatMessage"` is available on xStarbound and StarExtensions.**

xClient's engine sends this message locally to the primary player whenever a chat message is received, but may also be sent non-locally.

Additionally, when xClient's engine receives messages of these types, it invokes *all* Lua message handlers for them and concatenates their results into a `JsonArray`. `nil` results will be explicit `null`s, so you *can* use `jsize` on the array to find out how many handlers you've tripped.

The format of `"chatMessage"`'s argument is as follows:

```lua
jobject{
    mode = "CommandResult", -- The chat mode, any of "Local", "Party", "Broadcast", "Whisper", "CommandResult", "RadioMessage" or "World".
    channel = "", -- If the mode is "Party" or "Local", the channel name.
    connection = 0, -- The connection ID for the player who posted the message.
    -- Server messages have an ID of 0, while player connection IDs start at 1, going up. Can be ignored even if spoofing player messages.
    fromNick = "", -- The sender's nickname. If an empty string, no nick is shown in chat.
    portrait = "", -- The chat portrait. Is an empty string if there's no portrait. Not used for messages that don't have a "RadioMessage" mode.
    text = "", -- The chat message, of course. Replaced by the `text` argument if present, so this parameter can be ignored.
    showPane = true -- Whether to show the chat pane when the message is added.
    -- Note that there is no `scripted` parameter -- assume xClient always returns a `null` or `false` here.
}
```
The format of `"newChatMessage"`'s argument is as follows (for compatibility with StarExtensions mods):

```lua
jobject{
    mode = "Local", -- The chat mode, any of "Local", "Party", "Broadcast", "Whisper", "CommandResult", "RadioMessage" or "World".
    channel = "", -- If the mode is "Party" or "Local", the channel name.
    connection = 0, -- The connection ID for the player who posted the message.
    -- Server messages have an ID of 0, while player connection IDs start at 1, going up.
    nickname = "", -- The sender's nickname. If an empty string, no nick is shown in chat.
    portrait = "", -- The chat portrait. Is an empty string if there's no portrait. Not used for messages that don't have a "RadioMessage" mode.
    text = "" -- The chat message, of course.
}
```

You can disable handling non-local messages of this type for a given player on xClient with `player.setExternalRadioMessagesIgnored`; this does not mark the message as failed.

---

#### `Json` `commandName` [player] (`String` chatArguments)

**Available only on xStarbound, OpenStarbound and StarExtensions.**

xClient's engine sends this message locally to all players whenever the associated slash command, where `commandName` is the slash command before any whitespace, is sent. E.g., a `/foo` command in the chatbox will invoke the handler `"/foo"` if one is found.

`chatArguments` is a string containing all text following the command name (excluding the single space splitting the command name and this text).

Any JSON value may be returned and will be displayed as a chat message in response to the command, although it's best practice to return a string (which will be printed 'raw') for obvious reasons.

**Note:** This sort of message handler takes precedence above sending commands to the server. I.e., if you've set up a handler for `/who`, it will intercept any `/who` commands before they're sent to the server. To bypass client-side command handler processing, use `chat.send` (see `interface.md`) or `player.sendChat` (see `player.md`).

---

#### `void` `"control_on"` [vehicle] (`uint64_t` loungePosition, `String` control)

Toggles *on* a specified control for a specified lounging position on a vehicle. Normally entirely handled by the engine. This message is sent by a player or NPC lounging in and controlling a vehicle.

The possible controls are:

- `"Left"`
- `"Right"`
- `"Down"`
- `"Up"`
- `"Jump"`
- `"PrimaryFire"`
- `"AltFire"`
- `"Special1"`
- `"Special2"`
- `"Special3"`

#### `void` `"control_off"` [vehicle] (`uint64_t` loungePosition, `String` control)

Toggles *off* a specified control for a specified lounging position on a vehicle. Normally entirely handled by the engine. This message is sent by a player or NPC lounging in and controlling a vehicle.

See above for the possible controls.

#### `void` `"control_all"` [vehicle] (`uint64_t` loungePosition, `StringList` controlsOn)

Sets the toggled controls for a specified lounging position on a vehicle. Controls that are listed are toggled *on*, while those that are *not* listed are toggled *off*, all in one go. Normally entirely handled by the engine. This message is sent by a player or NPC lounging in and controlling a vehicle.

See above for the possible controls.

#### `void` `"aim"` [vehicle] (`uint64_t` loungePosition, `float` xPosition, `float` yPosition)

Sets the aim position a specified lounging position on a vehicle, as for a turret or similar. This message is sent by a player or NPC lounging in and controlling a vehicle.

#### `void` `"requestUpgrade"` [crafting interface source entity] ()

> **Note:** On xClient, the source entity of a crafting interface may now be any interactive entity. On other clients, the source entity *must* be a tile entity.

Tells the source entity of a crafting interface — normally a crafting station object — to upgrade itself, usually to the next upgrade stage.

Normally sent automatically by the client's engine. This message may be handled by a Lua script running on the entity so requested. On vanilla crafting stations (notwithstanding any changes by asset mods), the handling script is `$assets/objects/crafting/upgradeablecraftingobjects/upgradeablecraftingobject.lua`.

#### `void` `"startCrafting"` [container object] ()

Tells the container to start crafting. This is used for furnaces and other similar "smelters" or "extractors" that handle their crafting server-side. 

Normally entirely handled by the client and server engines.

#### `void` `"stopCrafting"` [container object] ()

Tells the container to stop crafting. This is used for furnaces and other similar "smelters" or "extractors" that handle their crafting server-side. 

Normally entirely handled by the client and server engines.

#### `void` `"burnContainerContents"` [container object] ()

Tells the container to convert its contents to ship fuel. Used for ship fuel hatches.

Normally entirely handled by the client and server engines. The client sends this message whenever the **Fuel** button is clicked in a fuel hatch.

Under the hood, this message stops any container crafting, calculates how much fuel value the contents holds, destroys as much of the contents as necessary to fill up the tank to the level set by the world property `"ship.maxFuel"`, and sets the world property `"ship.fuel"` to the resulting amount.

This message call *technically* doesn't actually care if it's running on a shipworld, but since anything that isn't a player shipworld (normally) has no `"ship.maxFuel"` property anyway (defaulting to `0`), it still effectively does nothing when called on an ordinary planet.

#### `ItemDescriptor` `"addItems"` [container object] (`ItemDescriptor` itemStack)

Attempts to add the given item stack to the first available slot(s) in the container, returning any overflow.

Normally entirely handled by the client and server engines. The client sends this message whenever an item is <kbd>Shift</kbd>-clicked into a container.

#### `ItemDescriptor` `"putItems"` [container object] (`uint64_t` slot, `ItemDescriptor` itemStack)

Attempts to put the given item stack in the specified container slot, returning any overflow.

Normally entirely handled by the client and server engines. The client sends this message whenever an item stack is left-clicked into an empty container slot.

#### `ItemDescriptor` `"takeItems"` [container object] (`uint64_t` slot, `uint64_t` count)

Takes the item stack in the specified container slot, returning said item stack.

Normally entirely handled by the client and server engines. The client sends this message whenever a player whose swap slot is empty or contains a matching item stack left- or right-clicks a container slot with items in it.

#### `ItemDescriptor` `"swapItems"` [container object] (`uint64_t` slot, `ItemDescriptor` itemStack, [`bool` tryCombine])

Swaps the item stack in the specified container slot with the specified item stack, returning the swapped stack (or nothing if there was nothing in the specified slot) unless the items are stackable with each other, in which case the overflow is returned if `tryCombine` is `true` or unspecified, or no items are taken and `nil` is returned otherwise.

Normally entirely handled by the client and server engines. The client sends this message whenever an item stack is left-clicked into a container slot that already has a non-matching item stack in it.

#### `ItemDescriptor` `"applyAugment"` [container object] (`uint64_t` slot, `ItemDescriptor` itemStack, [`bool` tryCombine])

Attempts to apply the specified augment stack to the item in the specified stack. If successful, applies the augment to the item in the specified slot, removes one augment item from the stack and returns the stack, or `nil` if there was only one augment in the stack. Otherwise returns the augment stack unchanged and does nothing.

Normally entirely handled by the client and server engines. The client sends this message whenever an augment is right-clicked into any slot with a non-matching item in it.


#### `bool` `"consumeItems"` [container object] (`ItemDescriptor` items)

Attempts to take items matching the specified descriptor out of the container, returning any taken item stack. The container must have at least the number of matching items specified in the descriptor's count (or at least one item if no count is specified), or otherwise no items will be taken and `nil` will be returned.

Normally entirely handled by the client and server engines. The client sends this message whenever `world.containerConsume` is invoked on a container.

#### `bool` `"consumeItemsAt"` [container object] (`uint64_t` slot, `uint64_t` count)

Attempts to take the specified number of items out of a stack in the specified slot, returning any taken item stack. Takes no items and returns `nil` if the stack has fewer than the specified number of items in it.

Normally entirely handled by the client and server engines. The client sends this message whenever `world.containerConsumeAt` is invoked on a container.

#### `List<ItemDescriptor>` `"consumeItemsAt"` [container object] (`uint64_t` slot, `uint64_t` count)

Attempts to take the specified number of items out of a stack in the specified slot, returning any taken item stack. Takes no items and returns `nil` if the stack has fewer than the specified number of items in it.

Normally entirely handled by the client and server engines. The client sends this message whenever the **Loot All** or **Clear** button is clicked on a container, followed by one or more `"putItem"` messages if it turns out the player didn't have enough inventory space for everything.