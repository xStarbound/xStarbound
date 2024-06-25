# `CommandProcessor`

These callbacks are available to server-side command processor scripts.

----

#### `Maybe<String>` CommandProcessor.adminCheck(`ConnectionId` connectionId, `String` actionDescription)

Checks whether the specified connection ID is authorised to perform admin actions and returns `nil` if authorisation is successful. If unauthorised, returns a `String` error message to display to the client requesting the action, which may include the specified action description, such as "Insufficient privileges to do the time warp again".

----

#### `Maybe<String>` CommandProcessor.localCheck(`ConnectionId` connectionId, `String` actionDescription)

> **Available only on xStarbound.**

Checks whether the specified connection ID is a client acting as the server host (i.e., the client is hosting a Steam or Discord multiplayer session) and returns `nil` if so. Otherwise returns a `String` error message to display to the client requesting the action, which may include the specified action description.