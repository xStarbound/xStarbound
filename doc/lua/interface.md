# `interface`, `input`, `voice`, `chat`, `clipboard` and `camera`

> **These callbacks are available only on xStarbound, OpenStarbound and StarExtensions.**

The `interface`, `input`, `voice`, `chat`, `clipboard` and `camera` tables contain bindings that perform actions that relate to the client window, voice chat, game input/output handling and the world camera.

These tables are available in the following contexts:

- universe client scripts
- generic player scripts
- player companion scripts
- player deployment scripts
- tech scripts
- client-side NPC scripts
- client-side monster scripts
- client-side status effect and controller scripts
- client-side vehicle scripts
- client-side projectile scripts
- client-side active and fireable item scripts
- active item animation scripts
- monster and object animation scripts
- pane scripts
- chat pane scripts (on xStarbound)
- container interface scripts

The `voice` table is also available in the following context:

- **Voice Settings** dialogue scripts

The `input` table is also available in the following context:

- **Mod Bindings** dialogue scripts

The following bindings are available across all five tables:

---

#### `Maybe<CanvasWidget>` interface.bindCanvas(`String` canvasName, [`bool` ignoreInterfaceScale])

Binds a canvas widget, returning a `CanvasWidget` object. If `ignoreInterfaceScale` is true, the returned `CanvasWidget` object's methods will return and take positional values in actual display pixels instead of scaled pixels.

---

#### `Maybe<LuaCallbacks>` interface.bindRegisteredPane(`String` registeredPaneName)

"Binds" one of the game's primary registered panes to a returned table of script pane callbacks; see `scriptpane.md` for documentation on these callbacks. The following are valid registered pane names:

- `"EscapeDialog"`
- `"Inventory"`
- `"Codex"`
- `"Cockpit"` (unused; will return `nil`)
- `"Tech"` (unused; will return `nil`)
- `"Songbook"`
- `"Ai"`
- `"Popup"`
- `"Confirmation"`
- `"JoinRequest"`
- `"Options"`
- `"QuestLog"`
- `"ActionBar"`
- `"TeamBar"`
- `"StatusPane"`
- `"Chat"`
- `"WireInterface"`
- `"PlanetText"`
- `"RadioMessagePopup"`
- `"CraftingPlain"`
- `"QuestTracker"`
- `"MmUpgrade"`
- `"Collections"`
- `"CharacterSwap"` (xStarbound and OpenStarbound)
- `"CharacterAdd"` (xStarbound only)
- `"CharacterRemove"` (xStarbound only)
- `"CharacterEdit"` (xStarbound only)

**Note:** On xClient, this callback will return `nil` in any Lua context where a callback table could unsafely linger after registered panes are deregistered (which happens whenever the client swaps players), i.e., it will return callbacks _only_ in pane and universe client scripts. If you need to use callbacks on a registered pane from any other script, set up a universe client script or dummy script pane and appropriate message handlers, or ask xStarbound maintainers to add scripting support to your favourite registered pane — chat panes already have such support.

---

#### `void` interface.displayRegisteredPane(`String` registeredPaneName)

Displays one of the game's registered panes. Uses the same registered pane names as `interface.bindRegisteredPane` above.

---

#### `float` interface.scale()

> **Available only on xStarbound and OpenStarbound. Returns an `int` on OpenStarbound.**

Returns the current interface scale, expressed as the number of interface pixels per screen pixel in either dimension.

---

#### `float` interface.setScale(`float` newScale)

> **Available only on xStarbound.**

Sets the interface scale. The scale is clamped to a value between `0.5` and `100.0`, inclusive.

---

#### `float` interface.worldPixelRatio()

#### `float` camera.pixelRatio()

> **`interface.worldPixelRatio` is available only on xStarbound.**

Returns the current world zoom level, expressed as the number of world pixels per screen pixel in either dimension.

---

#### `void` interface.setWorldPixelRatio(`float` newScale)

#### `void` camera.setPixelRatio(`float` newScale, `Maybe<bool>` smooth)

> **`interface.setWorldPixelRatio` is available only on xStarbound.**

Sets the world zoom level. The zoom level is clamped to a value between `0.25` and `100.0`, inclusive. Note that a lower zoom ratio will cause more world chunks to load.

`interface.setWorldPixelRatio` does a smooth zoom transition by default, while `camera.setPixelRatio` does not, although if `smooth` is `true`, it will do a smooth transition.

---

#### `Vec2F` interface.cameraPosition()

#### `Vec2F` camera.position()

Returns the current world position of the "camera", i.e., the centre of the game world view, in world tile coordinates.

---

#### `Vec2U` camera.screenSize()

Returns the size of the game window (not including any titlebar or other window decorations) in raw screen pixels. If the game is full-screen, returns the full-screen resolution.

---

#### `RectF` camera.worldScreenRect()

Returns the bounds of the game window or screen (not including any titlebar or other window decorations) in _world coordinates_. Useful for determining whether an entity is visible in game or not.

---

#### `RectI` camera.worldTileRect()

Returns the bounds of the game window or scereen (not including any titlebar or other window decorations) in _world tile coordinates_, including any partially visible tiles. Useful for determining whether a tile, object or other tile-positioned entity is visible in game or not.

---

#### `Vec2F` camera.worldTileRect()

Returns the position of the lower left corner of the lowermost and leftmost tile visible (even if only partially) on screen in _raw screen pixels_. The returned coordinates will be zero or negative in both axes — a screen pixel position that's usually just off-screen.

---

#### `Vec2F` camera.screenToWorld(`Vec2F` worldCoord)

Converts the given _world coordinates_ to _screen pixel coordinates_. The origin for the screen pixel coordinates is the lower-left corner of the game window or screen (excluding any window decorations).

---

#### `Vec2F` camera.worldToScreen(`Vec2F` worldCoord)

Converts the given _screen pixel coordinates_ to _world coordinates_. The origin for world coordinates is the point at which the world's transition seam — the line where the world's X coordinate «overflows» or «underflows» — intersects the bottom of the world.

---

#### `void` interface.overrideCameraPosition(`Vec2F` newPosition)

> **Available only on xStarbound. StarExtensions has `camera.override` instead; [see its documentation](https://github.com/StarExtensions/StarExtensions/blob/main/LUA.md) for details.**

Overrides the current position of the "camera" for a single tick. Unlike `player.overrideCameraPosition` (see `player.md`), this override prevents the **Camera Look** key from being used and takes effect when called from a script running on a secondary player.

---

#### `void` interface.queueMessage(`String` newMessage, [`float` cooldown], [`float` springState])

Queues a status message, visible at the bottom of the screen. If a cooldown is specified, the queued message will hold up "playback" of subsequent messages for that many seconds.

If a spring state value is specified, it represents the decimal percentage of the status message that will be immediately visible when it appears. If this value is less than `1.0`, the message will then gradually rise up to become fully visible.

Status messages queued with this callback on a secondary player will be shown the same way as if queued on the primary player — i.e., any queued messages start being "played back" in queue order immediately, no matter which player queued them.

Also see `player.queueStatusMessage` in `player.md`.

---

#### `void` interface.setCursorText(`Maybe<String>` cursorText, `Maybe<bool>` overrideGameTooltips)

> **Available only on xStarbound.**

Sets the cursor tooltip text. If any text is specified, the tooltip will be visible this tick. If `overrideGameTooltips` is `true`, the cursor text will replace the tooltips visible when hovering over the "toolbar" buttons (normally at the upper right corner of the game screen).

Note that with the default tooltip sprite at `$assets/interface/rightBarTooltipBg.png`, there isn't all that much space for text, so be terse. Or make the sprite larger with a mod — the engine bases tooltip alignment on the image's actual size.

---

#### `Maybe<StringList>` chat.command(`String` chatText, `Maybe<bool>` addToHistory, `Maybe<String>` sendMode)

#### `Maybe<StringList>` interface.doChat(`String` chatText, `Maybe<bool>` addToHistory, `Maybe<String>` sendMode)

Sends/enters a chat message exactly as if it were sent through the vanilla chat interface, returning any _client-side_ command results as a list of strings, or `nil` if no such results were returned.

If `addToHistory` is `true`, the sent/entered chat message is put in the player's sent chat message history (accessible with the arrow keys in the chat box).

The `sendMode`, if specified, can be either `"Broadcast"`, `"Local"` or `"Party"`. If not specified, the current chat send mode is used unless `chat.command` is used in a chat pane script, where `"Broadcast"` is always used as the default.

**Note:** `interface.doChat` invokes `sendMode` in chat pane scripts, so don't have `sendMode` invoke this callback to prevent a recursion error!

---

#### `void` interface.drawDrawable(`JsonObject` drawable, `Vec2F` screenPos, `float` pixelRatio, `Maybe<Vec4B>` colour)

Draws a drawable in the game window this tick. `drawable` is a JSON object with the following parameters:

```lua
jobject{
--- Choose *one* of the following keys. ---
    image = "/assetmissing.png?setcolor=ffff", -- With this key, this drawable will be an image.
    -- The value is an asset path to an image, optionally with a frame specifier and/or directives.
    line = { { 45, 67 }, { 557, 443 } }, -- With this key, this drawable will be a line.
    -- The value is an array of two `Vec2F`s determining the beginning and end of the line.
    poly = { { 45, 67 }, { 50, 560 }, -- With this key, this drawable will be a polygon.
             { 459, 5594 }, { 557, 120 } },
    -- The value is a `Poly` array of any number of `Vec2F`s that determines the points in the polygon.
    -- The polygon will be filled in with the specified `colour`.
--- The following key is *required* for lines. ---
    width = 1.0, -- The width of the line in screen pixels.
--- The following keys are *optional* and only apply to image drawables. ---
    transformation = { { 1.0, 0.0, 0.0 }, -- If specified, a 3×3 matrix transformation to be applied
                       { 0.0, 1.0, 0.0 }, -- to the image. Basically an array of three `Vec3F`s.
                       { 0.0, 0.0, 1.0 } },
--- If a transformation is specified, the *optional* keys for images below this line are ignored. ---
    centered = false, -- If `true`, centres the image drawable such that the centre is at the specified
                      -- `screenPos`. `true` if unspecified.
    mirrored = false, -- Mirrors the image over the Y axis if `true`. `false` if unspecified.
    scale = 1.5,      -- If specified, scales the image by the specified value.

}
```

The `screenPos` is the position of the drawable's "origin". For images, this is their centre (or lower-left corner if `centered` is `false`). For lines and polygons, this is the origin for all specified `Vec2` coordinates in the JSON object. The `screenPos` is _always_ in raw screen pixels and never adjusted for interface scale, but coordinates in the drawable JSON are _multiplied_ by the specified `pixelRatio`.

The `pixelRatio` scales the entire drawable by the given amount. As the name implies, this is normally used to scale the drawable to the interface scale (which can be gotten with `interface.scale`).

Lastly, if `colour` is specified, the drawable will be the specified RGBA colour. For images, specifying a `colour` is equivalent to executing a `?multiply` operation with the hex form of the chosen colour _after_ any specified directives. For lines and polygons, their colour is the specified `colour`, or a plain white (`{255, 255, 255, 255}`) if not specified.

The drawable will be rendered for one game tick, so this callback should generally be called every tick.

---

#### `Vec2U` interface.windowSize()

Returns the current size of the game window (or if full-screen, the current screen resolution) in screen pixels.

---

#### `Vec2U` interface.cursorPosition()

Returns the current position of the cursor relative to the lower-left corner of the game window (or if full-screen, the screen).

---

#### `bool` interface.hudVisible()

**Only available on xStarbound v3.4.5.1+ and OpenStarbound.**

Returns whether the interface HUD is visible.

---

#### `void` interface.setHudVisible(`bool` visible)

**Only available on xStarbound v3.4.5.1+ and OpenStarbound.**

Sets whether the interface HUD should be visible.

---

#### `void` chat.addMessage(`Maybe<String>` messageText, `Maybe<JsonObject>` chatMessageConfig)

> **Note:** The alias `interface.addChatMessage` was removed in xStarbound v3.4.5.1.

Adds a chat message visible only on this client. It takes two parameters — a JSON object conforming to the `"chatMessage"` format above (any omitted entries default to the values shown above) and an optional `bool` for whether the chat pane should be shown when the message is added. If no parameters are passed, the callback does nothing (rather than causing an error).

The `chatMessageConfig` must be in the following format:

```lua
jobject{
  context = jobject{
    mode = "Local", -- The chat mode, any of `"Local"`, `"Party"`, `"Broadcast"`, `"Whisper"`, `"CommandResult"`,
    -- `"RadioMessage"` or `"World"`. Defaults to `"Local"` for `interface.addChatMessage` or `"CommandResult"` for
    -- `chat.addMessage`.
    channel = "" -- If the mode is "Party" or "Local", the channel name. This is currently only used for party UUIDs.
  },
  connection = 0, -- The connection ID for the player who posted the message.
  -- Server messages have an ID of 0, while player connection IDs start at 1, going up.
  nick = "", -- The sender's nickname. If an empty string, no nick is shown in chat.
  portrait = "", -- The chat portrait. Is an empty string if there's no portrait.
  -- Not used for messages that don't have a `"RadioMessage"` mode.
  message = "", -- The chat message, of course.
  showPane = true -- Whether to show the chat pane. Optional; defaults to `true`.
  -- Ignored by a scripted chat pane.
}
```

`messageText`, if specified, is used instead of any message text in `chatMessageConfig`. The chat pane will be shown when the message is added.

Note that _either_ the chat text _or_ the chat message config must be specified in order to add a message. Passing `nil` in _both_ fields means nothing happens.

To actually _send_ messages, use `interface.doChat` (see above), `chat.command` (see above) `chat.send` (below) or `player.sendChat` (see `player.md`).

**Note:** This callback invokes `addMessages` in chat pane scripts, so don't have `addMessages` invoke this callback to prevent a recursion error!

---

#### `void` chat.send(`String` text, `Maybe<String>` sendMode, `Maybe<bool>` suppressBubble)

> _Note:_ On OpenStarbound, the `suppressBubble` parameter is instead `spawnBubble`, which if `true`, always spawns a chat bubble when the chat message is sent; if `false`, it never spawns one; if `nil` or unspecified, a chat bubble is spawned if the chat text doesn't begin with `/`. StarExtensions lacks a third parameter and has OpenStarbound's default chat bubble behaviour.

Sends a chat message, _skipping_ client-side command processing. Arguments are as follows:

- `text`: The text to send.
- `sendMode`: If specified, may be any one of `"Local"`, `"Broadcast"` or `"Party"` (anything else resolves to `"Broadcast"`). Defaults to `"Local"`.
- `suppressBubble`: If `true`, no chat bubble is spawned when the chat message is sent.

The message will be sent immediately regardless of whether the player is primary or secondary, but the chat bubble will always spawn above the _primary_ player.

Identical in functionality to `player.sendChat` (see `player.md`), except that `sendMode` defaults to `"Broadcast"`. This `chat` callback was originally added for compatibility with StarExtensions mods.

---

#### `List<String>` chat.parseArguments(`String` text, `String` sendMode, `bool` suppressBubble)

> \*Returns `List<Json>` on OpenStarbound and StarExtensions, since on those mods, this callback invokes different (and non-standard!) parsing of strings containing JSON object or array notation.

Parses a line of raw text into a list of substrings, handling whitespace and escape codes as if the arguments were passed to any halfway decent CLI shell (i.e., not Windows' CMD). More specifically, the following rules are used:

- Each chunk of text surrounded by unescaped quotes is taken to be one substring.
- Escaped spaces (`\ `) outside quoted text cause otherwise separate substrings to be glommed together.
- Other escape sequences are replaced with the characters they represent (e.g., `\n` becomes a newline).

This is useful for parsing arguments passed to command message handlers in a reasonably sane manner.

---

#### `String` chat.input()

Gets any text currently in the chat box. Returns `""` if the chat box isn't focussed, even if it contains any text. Nearly identical to `player.getChatText` (see `player.md`).

**Note:** This callback invokes `currentChat` in chat pane scripts, so don't have `currentChat` invoke this callback to prevent a recursion error!

---

#### `String` chat.mode()

Returns the chat sending mode currently selected in the chat box. This can be either `"Broadcast"`, `"Local"` or `"Party"`.

**Note:** This callback invokes `sendMode` in chat pane scripts, so don't have `sendMode` invoke this callback to prevent a recursion error!

---

#### `bool` chat.setInput(`String` newChatInput)

Sets the text in the chat input box to the specified value. Returns whether the chat input was set.

**Note:** This callback invokes `setCurrentChat` in chat pane scripts, so don't have `setCurrentChat` invoke this callback to prevent a recursion error!

---

#### `void` chat.clear(`Maybe<size_t>` numberOfMessages)

If `numberOfMessages` is not specified, clears the entire received message history, including whatever is saved to `$storage/messages.json`, up to the point this binding is invoked (once it gets saved again).

If `numberOfMessages` _is_ specified, clears only the last `n` messages, where `n` is the specified number, or all messages if there are `n` or fewer in the history.

**Note:** This callback invokes `clearMessages` in chat pane scripts, so don't have `clearMessages` invoke this callback to prevent a recursion error!

---

#### `bool` clipboard.hasText()

Returns whether the OS's clipboard currently has any (UTF-8) text on it.

**Note:** On the Linux build, this callback will return `false` if there are more than about 50,000 raw bytes of text on the clipboard. This is due to SDL2's lack of support for X11's incremental paste (and the equivalent on Wayland).

---

#### `Maybe<String>` clipboard.getText()

If the OS's clipboard has any text on it, returns the text. Otherwise returns `nil`.

**Note:** On the Linux build, this callback will return `nil` if the clipboard text is too long; see `clipboard.hasText`.

---

#### `Maybe<String>` clipboard.setText(`String` newClipboardText)

Sets the text on the OS clipboard. If successful, returns `nil`, but if there is any error, returns a string describing the error.

**Note:** On the Linux build, this callback won't set the clipboard text and will return an error if the text is too long; see `clipboard.hasText`.

---

#### `Maybe<uint32_t>` input.bindDown(`String` category, `String` bindId)

Returns whether the specified bind ID in the specified category has any of its binds initially pressed (but not held!) this tick. If so, the number of times any of the binds in question are pressed this tick is returned; otherwise, `nil` is returned.

---

#### `bool` input.bindHeld(`String` category, `String` bindId)

#### `bool` input.bind(`String` category, `String` bindId)

> _`input.bind` is not available on OpenStarbound._

Returns whether the specified bind ID in the specified category has any of its binds being held this tick.

---

#### `Maybe<uint32_t>` input.bindUp(`String` category, `String` bindId)

Returns whether the specified bind ID in the specified category has any of its binds released this tick. If so, the number of times any of the binds in question are released this tick is returned; otherwise, `nil` is returned.

---

#### `Maybe<uint32_t>` input.keyDown(`Key` key, `Maybe<List<KeyMod>>` mods)

Returns whether the specified key is initially pressed (but not held!) this tick. If so, the number of times the key is pressed this tick is returned; otherwise, `nil` is returned.

If any key modifiers are supplied (even an empty array), _all_ of the specified modifier keys, and _no_ others, must be held this tick for any presses to count; otherwise, `nil` is returned.

As a caveat, the states of Num Lock, Caps Lock and Scroll Lock are _ignored_ if those modifier keys aren't specified, so e.g., you _can_ hold down Caps Lock and still have the input "count", even if `"Caps"` isn't in the list.

**Note:** See _Keys, keymods, mouse buttons, etc._ below for a list of valid `Key` and `KeyMod` strings.

---

#### `bool` input.keyHeld(`Key` key)

#### `bool` input.key(`Key` key)

> _`input.key` is not available on OpenStarbound._

Returns whether the specified key is being held this tick.

---

#### `Maybe<uint32_t>` input.keyUp(`Key` key)

Returns whether the specified key is released this tick. If so, the number of times the key was released this tick is returned; otherwise, `nil` is returned.

---

#### `Maybe<List<Vec2I>>` input.mouseDown(`MouseButton` mouseButton)

Returns whether the specified mouse button is initially pressed (i.e., clicked, but not held!) at least once this tick. If so, a list of game screen cursor positions (with their origin at the lower left corner of the game window) where the specified button is initially clicked (one for each time the button is clicked) is returned. Otherwise, `nil` is returned.

**Note:** See _Keys, keymods, mouse buttons, etc._ below for a list of valid `MouseButton` strings.

---

#### `bool` input.mouseHeld(`MouseButton` mouseButton)

#### `bool` input.mouse(`MouseButton` mouseButton)

> _`input.mouse` is not available on OpenStarbound._

Returns whether the specified mouse button is being held this tick.

---

#### `Maybe<List<Vec2I>>` input.mouseUp(`MouseButton` mouseButton)

Returns whether the specified mouse button is released at least once this tick. If so, a list of game screen positions where the mouse button is released this tick (one for each release) is returned. Otherwise, `nil` is returned.

---

#### `void` input.resetBinds(`String` category, `String` bindId)

Resets the specified bind ID in the specified category to the default binds configured in its `.binds` file.

---

#### `void` input.setBinds(`String` category, `String` bindId, `JsonArray` newBindList)

Sets the specified bind ID in the specified category to the specified list of binds. The bind list must be a JSON array containing any number of binds. Example showing all possible bind types in a single array:

```lua
jarray{ -- Array of binds.
  jobject{
    type = "key", -- A keybind.
    value = "W", -- The key that must be pressed to activate the bind.
    -- See the `Key` names below for valid key names.
    mods = jarray{"LShift"} -- Here, *at least* the specified modifiers
    -- must be pressed or held to count. Extra modifiers are okay, and
    -- the state of the three lock keys is *completely* ignored even if
    -- they are specified in the modifiers array. Optional; if
    -- unspecified, defaults to an empty `jarray{}`. See the `KeyMod`
    -- names below for valid key modifier names.
  },
  {
    type = "mouse", -- A mouse bind.
    value = "MouseMiddle", -- The mouse button that must be
    -- clicked to activate the bind. See the `MouseButton` names below
    -- for valid mouse button names.
    mods = jarray{"LShift"} -- Same as above.
  },
  {
    type = "controller", -- A controller button bind. Binds of this type
    -- are currently useless because xClient does not check controller
    -- button presses at all.
    value = "Y" -- The controller button that must be pressed to
    -- activate the bind. See the `ControllerButton` names below
    -- for valid controller button names.
    controller = 0 -- The controller which must be used to activate
    -- the bind. Starts from 0. If you have multiple controllers
    -- plugged in, their order is dependent on your system
    -- configuration.
  }
}
```

---

#### `JsonArray` input.getDefaultBinds(`String` category, `String` bindId)

Gets the default binds for the specified bind ID, as configured in its `.binds` file. Returns the binds in the same format used by `input.setBinds` for setting binds.

---

#### `JsonArray` input.getBinds(`String` category, `String` bindId)

Gets the current binds for the specified bind ID, as configured in `$storage/xclient.config`. Returns the binds in the same format used by `input.setBinds` for setting binds.

---

#### `JsonArray` input.events()

Returns all game input events that have happened this tick.

Example return value showing all possible event types:

```lua
jarray{
  jobject{
    type = "KeyDown" -- Event type for a key being pressed (but not held!)
    -- this tick.
    data = jobject{
      key = "S", -- The key that is pressed. Is a `Key` value (see below).
      mods = jarray{"LShift"} -- Any modifier keys held while the key above was pressed. -- Note that modifier key presses and releases get "logged" as their own events.
      -- The modifiers are `KeyMod` values (see below).
    }
  },
  jobject{
    type = "KeyUp" -- Event type for a key being released this tick.
    data = jobject{
      key = "S" -- The key that is released. Is a `Key` value (see below).
    }
  },
  jobject{
    type = "MouseButtonDown" -- Event type for a mouse button being pressed
    -- (i.e., clicked, but not held!) this tick.
    data = jobject{
      mouseButton = "MouseLeft", -- The mouse button that is pressed. Is a
      -- `MouseButton` value (see below).
      mousePosition = jarray{923, 534} -- The game screen position where the
      -- mouse click happened, in screen pixels relative to the lower left
      -- corner of the game window (or screen when it's full-screen).
    }
  },
  jobject{
    type = "MouseButtonUp" -- Event type for a mouse button being released
    -- this tick.
    data = jobject{
      mouseButton = "MouseLeft", -- The mouse button that is released. Is a
      -- `MouseButton` value (see below).
      mousePosition = jarray{927, 528} -- Ditto.
    }
  },
  jobject{
    type = "MouseWheel" -- Event type for a mouse button being released
    -- this tick.
    data = jobject{
      mouseWheel = 1, -- The scroll direction. `1` for up, `-1` for down.
      mousePosition = jarray{926, 530}, -- Ditto.
      mouseWheelName = "MouseWheelUp" -- The `MouseWheel` value for the
      -- scroll direction (see below).
    }
  },
  jobject{
    type = "MouseWheel" -- Event type for the mouse being moved this tick.
    data = jobject{
      mouseMove = jarray{3, -2}, -- The relative position of the mouse
      -- compared to its position last tick, in screen pixels.
      mousePosition = jarray{926, 530} -- Ditto.
    }
  }
}
```

---

#### `StringList` voice.devices()

Lists all available microphone devices detected by xClient.

---

#### `JsonObject` voice.getSettings()

Returns the current voice settings. The settings are in the following format:

```lua
jobject{
  enabled = false, -- Whether voice chat is enabled.
  deviceName = "GeneriCo System Microphone", -- The microphone device to use for
  -- voice chat.
  threshold = -45.0, -- The voice activity threshold in decibels.
  inputVolume = 1.0, -- Your mic's volume, as a decimal percentage.
  outputVolume = 0.75, -- The volume for everybody else's voices (or whatever
  -- audio they play through their "mics") in game, as a decimal percentage.
  inputMode = "PushToTalk", -- Either `"VoiceActivity"` or `"PushToTalk"`.
  channelMode = "Stereo", -- Either `"Mono"` or `"Stereo"`.
  loopback = false -- Whether xClient plays back your own lovely
  -- voice as you talk. A `bool`. Currently not shown or configurable in the
  -- current Voice Settings interface.
  version = 1 -- The voice settings version.
}
```

These voice settings are saved under the `"voice"` key in `$storage/xclient.config`.

---

#### `void` voice.mergeSettings(`JsonObject` newSettings)

Changes the voice chat settings whose keys are specified in the JSON object passed to this callback. See `voice.getSettings` for the settings format. All settings keys are optional, and the `version` key cannot be changed with this callback.

Settings changes take effect immediately.

---

#### `JsonObject` voice.speaker(`Maybe<SpeakerId>` speakerId)

Gets current information about the specified speaker, or this client as a speaker if no speaker ID is specified. A `SpeakerId` is an unsigned integer representing the speaker's connection ID.

The returned information is a JSON object with the following format:

```lua
jobject{
  speakerId = 3, -- The speaker ID, of course.
  entityId = −196608, -- The speaker's current primary player entity ID,
  -- or `0` if the speaker has no player entities rendered.
  name = "FezzedOne", -- The speaker's current primary player name.
  playing = true, -- `true` if the speaker is speaking (or leaving his
  -- mic open), `false` otherwise.
  muted = false, -- `true` if the speaker is muted on this client,
  -- `false` otherwise.
  decibels = 49.5655, -- The speaker's current volume in decibels.
  -- Adjusted for the distance between the speaker's primary player
  -- and the *closer* of this client's primary player and
  -- the centre of this client's camera view.
  smoothDecibels = 42.3443, -- Same as above, but with interpolated
  -- smoothing based on the speaker's "decibel history" over the
  -- last 10 ticks.
  position = jarray{456.545, 892.442} -- The world position of the
  -- speaker's primary player entity, or `jarray{0, 0}` if no such
  -- entity is currently rendered.
}
```

If a speaker ID the client hasn't seen before is specified, the returned information will be for a speaker with `"Unnamed"` for a name, a speaker ID of `0`, entity ID of `0` and a position of `{0, 0}`.

**Note:** If a speaker is using xClient, the player entity associated with the speaker is always his client's current primary player, not any secondaries.

---

#### `List<JsonObject>` voice.speakers(`Maybe<bool>` onlyPlaying)

If `onlyPlaying` is `true`, `nil` or unspecified, returns a list of speakers detected by the client who are currently speaking (or leaving their mics open).

If `onlyPlaying` is `false`, returns a list of all speakers the client has ever detected since connecting to the server; this is reset upon disconnection.

The list entries are in the speaker format returned by `voice.speaker`.

---

#### `bool` voice.speakerMuted(`SpeakerId` speakerId)

Returns whether the specified speaker is locally muted.

---

#### `void` voice.setSpeakerMuted(`SpeakerId` speakerId, `bool` muted)

Sets whether the specified speaker is locally muted.

---

#### `float` voice.speakerVolume(`SpeakerId` speakerId)

Returns the specified speaker's local volume.

---

#### `void` voice.setSpeakerVolume(`SpeakerId` speakerId, `float` volume)

Sets the specified speaker's local volume.

---

#### `Vec2F` voice.setSpeakerVolume(`SpeakerId` speakerId, `float` volume)

Gets the specified speaker's world position. Returns `{0, 0}` if the speaker hasn't been seen before, doesn't exist or has no currently rendered primary player.

**Technical note:** If the speaker _had_ a rendered primary player but now doesn't, the returned position is actually that primary player's last position before the player disappeared. This also applies to the position in speaker entries returned by `voice.speaker` and `voice.speakers`.

---

## Chat pane scripts

> **Only available on OpenStarbound v0.1.8+ and xStarbound v3.4.5.1+.**

If a `"scripts"` array is present in `$assets/interface/chat/chat.config`, the game will run a scripted chat pane as configured in that file instead of the default engine-controlled chat pane. When a scripted pane is used (on xStarbound), the engine does not expect any of the widgets it would normally expect for the engine-controlled pane. The following functions are called by the engine when running chat pane scripts, with their expected function signatures.

- **`void` addMessages(`List<JsonObject>` messages, `bool` showPane):** Invoked when the engine adds a message to the chat pane. Message objects are in the same format used by `chat.addMessage` and `interface.addChatMessage` (see above). This function should add the messages to your scripted chat pane's chat history view (and maybe persistently save them) and handle showing the chat pane if needed.
- **`void` startChat():** Invoked when the user presses the **Start Chat** (`"ChatBegin"`) keybind (which defaults to **<kbd>Enter</kbd>**).
- **`void` startCommand():** Invoked when the user presses the **Start Command** (`"ChatBeginCommand"`) keybind (which defaults to **<kbd>/</kbd>**).
- **`void` stopChat():** Invoked when the user presses the **Stop Chat** (`"ChatStop"`) keybind (which defaults to **<kbd>Esc</kbd>**) or the **Send Chat** (`"ChatSendLine"`) keybind (which defaults to **<kbd>Enter</kbd>**). This function should handle de-focussing the chat pane.
- **`Maybe<bool>` hasFocus():** Invoked whenever the engine wants to check whether the chat box (the text box where the user enters a chat message) has focus; a returned `nil` is assumed `false`. This function is executed every tick by xStarbound to check whether the chat pane is focussed, and the returned result will be passed to `player.chatHasFocus` (see `player.md`).
- **`Maybe<bool>` setCurrentChat(`String` newChat):** Invoked whenever `chat.setInput` (see above) is invoked in another script; a returned `nil` is assumed `false`. The expected return value should indicate whether the chat input has been set.
- **`Maybe<String>` currentChat():** Invoked whenever `chat.input` (see above) or `player.getChatText` (see `player.md`) is invoked in another script. The expected return value should be whatever is in the chat box. A returned `nil` is assumed to be an empty string (`""`).
- **`void` clearCurrentChat():** Invoked when the user presses the **Send Chat** (`"ChatSendLine"`) keybind (which defaults to **<kbd>Enter</kbd>**). This function should handle clearing the chat text in the chat box. Note that the engine sends the actual chat message just before invoking this function, so you don't need to call `interface.doChat`, `chat.send` or `chat.command` yourself.
- **`void` clearMessages(`Maybe<int>` numberOfMessages):** Invoked when the user uses the `/clear` command or a script invokes `chat.clear`. The function should clear chat history, optionally with support for clearing only the last N messages from history if so specified.
- **`Maybe<String>` sendMode():** Invoked whenever the engine wants to know the current chat mode or when `chat.mode` is invoked in another script. The return value must be either `"Broadcast"`, `"Local"` or `"Party"`; if it's `nil` or any other string, xStarbound will assume it's `"Broadcast"`.
- **`Maybe<float>` visible():** Invoked whenever the engine wants to know how transparent the chat box ought to be. If the returned value is less than 0.1, the engine considers the chat pane «invisible» for the purpose of determining whether to render radio messages. If `nil` is returned, the engine assumes a transparency value of 1.0 (fully «opaque»).

Any missing function with a non-`void` return value will cause the engine to assume it has returned with a `nil` and use an appropriate default value.

---

## Keys, keymods, mouse buttons, etc.

There are a large number of valid string names for keys, keymods, mouse buttons, mouse scroll wheel actions, controller axes and controller buttons on xStarbound. Here's a list of them all, broken down by type name:

### `Key`

- `"Backspace"`
- `"Tab"`
- `"Clear"`
- `"Return"`
- `"Esc"`
- `"Space"`
- `"!"`
- `'"'` (double quote; note the surrounding single quotes)
- `"#"`
- `"$"`
- `"&"`
- `"'"` (single quote; note the surrounding double quotes)
- `"("`
- `")"`
- `"*"`
- `"+"`
- `","`
- `"-"`
- `"."`
- `"/"`
- `"0"`
- `"1"`
- `"2"`
- `"3"`
- `"4"`
- `"5"`
- `"6"`
- `"7"`
- `"8"`
- `"9"`
- `":"`
- `";"`
- `"<"`
- `"="`
- `">"`
- `"?"`
- `"@"`
- `"["`
- `"\\"` (single backslash, but must be escaped in Lua)
- `"]"`
- `"^"` (caret)
- `"_"`
- ``"`"`` (backtick)
- `"A"`
- `"B"`
- `"C"`
- `"D"`
- `"E"`
- `"F"`
- `"G"`
- `"H"`
- `"I"`
- `"J"`
- `"K"`
- `"L"`
- `"M"`
- `"N"`
- `"O"`
- `"P"`
- `"Q"`
- `"R"`
- `"S"`
- `"T"`
- `"U"`
- `"V"`
- `"W"`
- `"X"`
- `"Y"`
- `"Z"`
- `"Del"`
- `"Kp0"` (`Kp` means "numerical keypad")
- `"Kp1"`
- `"Kp2"`
- `"Kp3"`
- `"Kp4"`
- `"Kp5"`
- `"Kp6"`
- `"Kp7"`
- `"Kp8"`
- `"Kp9"`
- `"Kp_period"`
- `"Kp_divide"`
- `"Kp_multiply"`
- `"Kp_minus"`
- `"Kp_plus"`
- `"Kp_enter"`
- `"Kp_equals"`
- `"Up"`
- `"Down"`
- `"Right"`
- `"Left"`
- `"Ins"`
- `"Home"`
- `"End"`
- `"PageUp"`
- `"PageDown"`
- `"F1"`
- `"F2"`
- `"F3"`
- `"F4"`
- `"F5"`
- `"F6"`
- `"F7"`
- `"F8"`
- `"F9"`
- `"F10"`
- `"F11"`
- `"F12"`
- `"F13"`
- `"F14"`
- `"F15"`
- `"NumLock"`
- `"CapsLock"`
- `"ScrollLock"`
- `"RShift"`
- `"LShift"`
- `"RCtrl"`
- `"LCtrl"`
- `"RAlt"`
- `"LAlt"`
- `"RGui"`
- `"LGui"`
- `"AltGr"`
- `"Compose"`
- `"Help"`
- `"PrintScreen"`
- `"SysReq"`
- `"Pause"`
- `"Menu"`
- `"Power"`

### `KeyMod`

- `"NoMod"`
- `"LShift"`
- `"RShift"`
- `"LCtrl"`
- `"RCtrl"`
- `"LAlt"`
- `"RAlt"` (many keyboards have <kbd>Alt Gr</kbd> instead)
- `"LMeta"` (Left Windows key on Windows, Command key on Apple keyboards)
- `"RMeta"` (Right Windows key on Windows)
- `"Num"`
- `"Caps"`
- `"AltGr"` (Right Alt key on many keyboards)

### `MouseButton`

- `"MouseLeft"`
- `"MouseMiddle"` (scroll wheel on most mice)
- `"MouseRight"`
- `"MouseFourth"` (for mice with extra buttons; test which button is which before assigning)
- `"mouseFifth"` (again, for mice with extra buttons)

### `MouseWheel`

- `"MouseWheelUp"` (equivalent to `1`)
- `"MouseWheelDown"` (equivalent to `-1`)

### `ControllerAxis`

- `"LeftX"`
- `"LeftY"`
- `"RightX"`
- `"RightY"`
- `"TriggerLeft"` (how much the left trigger is depressed)
- `"TriggerRight"` (how much the right trigger is depressed)

### `ControllerButton`

- `"A"` (cross button on a PlayStation controller)
- `"B"` (circle button on a PlayStation controller)
- `"X"` (square button on a PlayStation controller)
- `"Y"` (triangle button on a PlayStation controller)
- `"Back"`
- `"Guide"`
- `"Start"`
- `"LeftStick"`
- `"RightStick"`
- `"LeftShoulder"`
- `"RightShoulder"`
- `"DPadUp"`
- `"DPadDown"`
- `"DPadLeft"`
- `"DPadRight"`
- `"Misc1"` (share, capture or microphone button, depending on controller)
- `"Paddle1"` (P1 paddle on an Xbox Elite controller)
- `"Paddle2"` (P3 paddle on an Xbox Elite controller)
- `"Paddle3"` (P2 paddle on an Xbox Elite controller)
- `"Paddle4"` (P4 paddle on an Xbox Elite controller)
- `"Touchpad"` (touchpad on a PS4 or PS5 controller)
- `"Invalid"` (your controller is broken or defective if it has one of these)

