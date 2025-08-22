# `universe`

> **These callbacks are available only on xStarbound and OpenStarbound.**

These callbacks are available in the following script contexts:

- command processor / universe server scripts
- world context scripts

**Note:** A `ClientId` is an unsigned integer.

---

#### `Json` universe.getServerData(`String` key)

#### `Json` universe.getServerDataPath(`String` path)

Gets the server data value located at the specified JSON key or path, if any, or returns `nil` if it's not found. See _JSON paths_ in `root.md` for more information on JSON paths. xStarbound server data is stored as versioned JSON with a `ServerData` identifier at `$storage/universe/server.dat`.

#### `void` universe.setServerData(`String` key, `Maybe<Json>` value)

#### `void` universe.setServerDataPath(`String` path, `Maybe<Json>` value)

Sets the server data value located at the specified JSON key or path to the specified value. See _JSON paths_ in `root.md` for more information on JSON paths. xStarbound server data is stored as versioned JSON with a `ServerData` identifier at `$storage/universe/server.dat`.

A `null` or `json.null` argument sets the value at the specified path (if a path can be found) to `null`. A `nil` argument removes the value at that specified path. When modifying a value in a JSON array, this difference will result in different-looking arrays!

Additionally, if an empty path is passed and the value is not a JSON object, the callback does nothing; this is intentional so that the server data's «root» is always a JSON object.

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

#### `void` universe.adminBroadcast(`String` message, `Maybe<JsonObject>` metadata)

> **The optional `metadata` parameter is only supported on xStarbound v3.5.3+.**

> _Note:_ Sending, transporting and receiving chat metadata requires xStarbound v3.5.3+ on _both_ the receiving client(s) _and_ on the server, _all_ running in xStarbound networking mode (not legacy mode!). Due to network compatibility issues, sending, transporting and receiving OpenStarbound v0.1.9+ chat metadata are _not_ supported by xStarbound!

Broadcasts the given server message to all connected clients, optionally with the specified chat message metadata.

#### `void` universe.adminWhisper(`ClientId` clientId, `String` message, `Maybe<JsonObject>` metadata)

> **The optional `metadata` parameter is only supported on xStarbound v3.5.3+.**

> _Note:_ Sending, transporting and receiving chat metadata requires xStarbound v3.5.3+ on _both_ the receiving client(s) _and_ the server, _all_ running in xStarbound networking mode (not legacy mode!). Due to network compatibility issues, sending, transporting and receiving OpenStarbound v0.1.9+ chat metadata are _not_ supported by xStarbound!

Whispers the given server message to the specified client, optionally with the specified chat message metadata.

#### `bool` universe.isAdmin(`ClientId` clientId)

Returns whether the given client ID is flagged as an admin. Always returns `false` on a client ID that isn't connected.

#### `bool` universe.isPvp(`ClientId` clientId)

Returns whether the given client ID is currently in PvP mode. Always returns `false` on a client ID that isn't connected.

#### `void` universe.setPvp(`ClientId` clientId, `Maybe<bool>` newPvpSetting)

Sets the PvP status of the given client. If `newPvpSetting` is unspecified, `true` or `nil`, it enables PvP for the client; if `false`, it disables PvP.

#### `bool` universe.isLocal(`ClientId` clientId)

Returns whether the given client is also the server. This is the case when the client is hosting a Steam or Discord multiplayer session.

#### `bool` universe.isWorldActive(`WorldId` worldId)

Returns whether the specified world is active. The argument must be a valid world ID string. Will return `false` if the world doesn't exist.

#### `List<WorldId>` universe.activeWorlds()

Returns a list of loaded worlds on the server.

#### `RpcPromise<Json>` universe.sendWorldMessage(`WorldId` worldId, `String` message, `Json...` args)

Sends a message to a world's global script context. The world's global script(s) must have a message handler to handle the message, exactly as for `world.sendEntityMessage`.

See `message.md` for information on message handlers and `RpcPromise` objects.

#### `WorldId` universe.clientWorld(`ClientId` clientId)

Returns the world the given client is located on. If the client isn't connected, returns `"Nowhere"`.

#### `Maybe<Uuid>` universe.clientUuid(`ClientId` clientId)

Returns the UUID of the client (as a string). The returned UUID is the true canonical UUID of the client, not a vanity UUID. Note that if the client has used `/swap` or `/swapuuid`, the returned UUID will be that of the client's shipworld. If the client isn't connected, returns `nil`.

#### `void` universe.disconnectClient(`ClientId` clientId, `Maybe<String>` reason)

> **Available only on xStarbound v3.6.2+ and OpenStarbound v0.1.10+.**

Disconnects (i.e., kicks) the specified client from the server. If a reason is specified, this reason is shown to the client upon disconnection.

### `void` universe.banClient(`ClientId` clientId, `Maybe<String>` reason, `Maybe<bool>` banByIp, `Maybe<bool>` banByUuid, `Maybe<int>` banTimeout)

> **Available only on xStarbound v3.6.2+ and OpenStarbound v0.1.10+.**

Bans the specified client from the server. If a reason is specified, this reason is shown to the client upon banning. If `banByIp` is `true`, the client's IP address will be added under `"bannedIPs"` in the server config (`$storage/xserver.config` on xServer, `$storage/starbound_server.config` on other servers); if `banByUuid` is `true`, the client will be added under `"bannedUuids"` in the server config. If neither `banByIp` or `banByUuid` is true, the «ban» is equivalent to a kick. If `banTimeout` is specified, the ban will expire after a specified number of seconds from the invocation of this callback.

### `Maybe<String>` universe.clientAccount(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns the name of the server account under which the specified client is connected, or `nil` if the client is connected anonymously or is not connected.

### `Maybe<String>` universe.canBeAdmin(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns `true` if the specified connected client is allowed to use `/admin` (regardless of the client's current admin status) or `false` otherwise.

### `Maybe<String>` universe.isGuest(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns `true` if the specified connected client is connected anonymously or under a configured guest account (an account in `xserver.config` or the host's `xclient.config` whose `"guest"` entry, if present, is `true`), or `false` otherwise.

### `Maybe<bool>` universe.hasBuildPermission(`ClientId` clientId, `Maybe<Vec3I>` systemLocation)

> **Available only on xStarbound v4.0+.**

Returns `true` if the specified connected client is allowed to spawn worlds or stations in a given system, `false` if the client is not allowed to do so, or `nil` if the client is not connected. Note that the callback can be run on star systems that don't exist, with no ill effects other than (most likely) a `false` return (or `true` if the system coordinates show up in `xserver.config` or the host's `xclient.config` anyway!). See `$docs/permissions.md` for more on xStarbound v4.0's build permission system.

### `void` universe.sendChat(`ClientId` clientId, `Json` chatMessage)

> **Available only on xStarbound v4.0+.**

Sends the specified chat message to the specified client, if online. Nothing happens if the specified client ID is offline.

Although this is the server-side equivalent of `chat.send` (see `interface.md`), the chat message object must conform to the format used by `chat.addMessage` (see that callback in `interface.md` for details), as this command skips the server-side processing used to determine which clients to relay a chat message to.

### `Maybe<String>` universe.clientTeam(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns the specified client's team UUID if the client is currently in a team; otherwise, it returns `nil`. All members of a given team have the same team UUID. The team UUID is internally used in chat message contexts and other server-side code to verify which clients a given client is teamed up with. Can be used, for example, in a custom player listing command that shows all teams on the server.

### `void` universe.warpClient(`ClientId` clientId, `String` warpAction)

> **Available only on xStarbound v4.0+.**

Warps the given client to the given location. This is the server-side equivalent of `player.warp` (see `player.md`). Note that clients set up to ignore warp messages cannot ignore warps done by this callback. If the client is offline, this callback does nothing.

As an example, this callback can be used in world server scripts to enforce claimed world whitelists or blacklists (alongside `universe.clientReturnWarp` and `universe.clientReviveWarp` to verify that a `Return` or death warp does not point to the same location, in order to avoid an infinite warp cycle and soft-lock there).

### `void` universe.flyClientShip(`ClientId` clientId, `Vec3I` systemCoordinate, `SystemLocation` systemLocation)

> **Available only on xStarbound v4.0+.**

Flies the given client's ship to the given location. This is the server-side equivalent of `celestial.flyShip` (see `celestial.md` for more information on system location formats). Does nothing if the client is not online. Suitable for a `/spawn` command or similar.

### `Maybe<CelestialCoordinate>` universe.clientShipCoordinate(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns the client's current ship coordinate in celestial coordinate format (see `celestial.md` for more information on celestial coordinates), or `nil` if the specified client is not online.

### `Maybe<SystemLocation>` universe.clientShipLocation(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns the client's current system location, or `nil` if the client's ship is currently flying to a different location or specified client is not online.

### `Maybe<String>` universe.clientReturnWarp(`ClientId` clientId)

### `Maybe<String>` universe.clientReviveWarp(`ClientId` clientId)

> **Available only on xStarbound v4.0+.**

Returns the client's current return or revive warp location, respectively, if the client is online and currently has any such warp location.

The return warp is used when the client executes any `Return` warp action. This normally happens when the client uses a «Return» action on various instance world teleporters. The revive warp is used to determine where the client should respawn after dying (on its main player for an xStarbound client) on some instance worlds.

### `void` universe.setClientReturnWarp(`ClientId` clientId, `String` warpAction)

### `void` universe.setClientReviveWarp(`ClientId` clientId, `String` warpAction)

> **Available only on xStarbound v4.0+.**

Sets the client's return or revive warp, respectively. Any warp location that is not `Nowhere`, an `InstanceWorld`, a `CelestialWorld` or `ClientShipWorld` (complete with an optional `=X.Y` world spawn coordinate) is ignored by these callbacks, as they do not really make sense for a spawn location.

If you want players to respawn at the equivalent of a `Player` location, consider using `universe.clientWorld` and a universe-to-world message handler to get the XY world coordinates of the player in question.
