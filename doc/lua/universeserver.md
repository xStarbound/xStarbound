# `universe`

These callbacks are available in the following script contexts:

- universe server scripts
- world context scripts

**Note:** A `ClientId` is an unsigned integer.

---

#### `List<ClientId>` universe.clientIds()

Returns a list of connected client IDs.

#### `uint64_t` universe.numberOfClients()

Returns the number of connected clients.

#### `bool` universe.isConnectedClient(`ClientId` clientId)

Returns whether the given client ID is connected.

#### `Maybe<String>` universe.clientNick(`ClientId` clientId)

Returns the nickname for the given client ID, or `nil` if the client is not connected.

#### `Maybe<ClientId>` universe.findNick(`String` nick)

Returns the client ID associated with the given nick, or `nil` if no matching client is found.

#### `void` universe.adminBroadcast(`String` message)

Broadcasts the given server message to all connected clients.

#### `void` universe.adminWhisper(`ClientId` clientId, `String` message)

Whispers the given server message to the specified client.

#### `bool` universe.isAdmin(`ClientId` clientId)

Returns whether the given client ID is flagged as an admin.

#### `bool` universe.isPvp(`ClientId` clientId)

Returns whether the given client ID is currently in PvP mode.

#### `void` universe.setPvp(`ClientId` clientId, `Maybe<bool>` newPvpSetting)

Sets the PvP status of the given client. If `newPvpSetting` is unspecified, `true` or `nil`, it enables PvP for the client; if `false`, it disables PvP.

#### `bool` universe.isLocal(`ClientId` clientId)

Returns whether the given client is also the server. This is the case when the client is hosting a Steam or Discord multiplayer session.

#### `bool` universe.isWorldActive(`WorldId` worldId)

Returns whether the specified world is active. The argument must be a valid world ID string. Will return `false` if the world doesn't exist.

#### `List<WorldId>` universe.activeWorlds()

Returns a list of loaded worlds on the server.

#### `RpcThreadPromise<Json>` universe.sendWorldMessage(`WorldId` worldId, `String` message, `Json...` args)

Sends a message to a world's global script context. The world's global script(s) must have a message handler to handle the message, exactly as for `world.sendEntityMessage`. See `message.md` for info on message handlers.

#### `WorldId` universe.clientWorld(`ClientId` clientId)

Returns the world the given client is located on. If the client isn't connected, returns `"Nowhere"`.

#### `Maybe<Uuid>` universe.clientUuid(`ClientId` clientId)

Returns the UUID of the client (as a string). The returned UUID is the true canonical UUID of the client, not a vanity UUID. Note that if the client has used `/swap` or `/swapuuid`, the returned UUID will be that of the client's shipworld. If the client isn't connected, returns `nil`.
