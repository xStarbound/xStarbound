# `message`

The `message` table contains a single callback, `setHandler`, which allows entities to receive messages sent using `world.sendEntityMessage` or `universe.sendWorldMessage`. Entities which can receive messages include:

- monsters
- NPCs
- objects
- vehicles
- stagehands
- projectiles

Additionally, messages can be handled by a variety of script contexts that run on players:

- active item scripts
- other item scripts
- quest scripts
- player companion scripts
- status scripts

Lastly, world server scripts can receive world messages, but cannot receive entity messages. For communication between a world server script and server-side scripted entities on that world, use `world.callScriptedEntity` for world-to-entity communication and `world.callScriptContext` for entity-to-world communication.

---

#### `void` message.setHandler(`String` messageName, `LuaFunction` handler)

Messages of the specified message type received by this script context will call the specified function. The first two arguments passed to the handler function will be the `String` `messageName` and a `bool` `isLocal`, indicating whether the message is from a local entity, followed by any arguments sent with the message. Note that `isLocal` is always true for world messages.
