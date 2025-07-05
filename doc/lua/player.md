# `player`

The `player` table contains functions with privileged access to the player. On xStarbound, this table is available in all script contexts which run on a client-mastered player. On other clients, this availability is more limited.

Availability of player callbacks is as follows:

- generic player scripts
- player deployment scripts
- player companion scripts
- active item scripts running on a player
- fireable item scripts running on a player (only on xStarbound)
- status controller and effect scripts running on a player (only on xStarbound)
- quest scripts
- tech scripts (only on xStarbound)
- pane scripts

As such, a modder on xStarbound may check for the existence of this table in scripts running on active items, status effects, etc., to determine whether the entity that is wielding the item, is affected by the status effect, etc., is a player.

> **Primary and secondary players:** On xStarbound, player callbacks normally apply to the (parent) player on which the script is running. As an exception, player callbacks invoked in pane scripts always apply to the primary player, as that is the only player which is associated with the game interface. Use entity messages or global variables if you wish to communicate with or invoke anything on secondary players in a pane script.

---

#### `Json` player.save()

#### `Json` player.getPlayerJson()

Returns a JSON object containing the contents of the entire player file. May be converted to a string with `sb.printJson`.

---

#### `void` player.load()

Loads a player from the specified JSON save. If any loading errors occur, the existing loaded player data is left unchanged. Note that this will _immediately_ replace any values in player script `storage` tables with the ones in the loaded save. To parse a string containing valid JSON, use `sb.parseJson`.

---

#### `EntityId` player.id()

Returns the player's entity ID.

---

#### `String` player.uniqueId()

Returns the player's UUID.

---

#### `String` player.isAdmin()

Returns whether the player is a client-side admin.

---

#### `Json` player.getProperty(`String` name, `Json` default)

Returns the value assigned to the specified generic player property. If there is no value set, returns the given default.

---

#### `void` player.setProperty(`String` name, `Json` value)

Sets a generic player property to the specified value.

---

#### `void` player.addScannedObject(`String` name)

Adds the specified object to the player's scanned objects.

---

#### `void` player.removeScannedObject(`String` name)

Removes the specified object from the player's scanned objects.

---

#### `void` player.interact(`String` interactionType, `Json` config, [`EntityId` sourceEntityId])

Triggers an interact action on the player as if the player had initiated an interaction and the result had returned the specified interaction type and configuration. Can be used to e.g. open GUI windows normally triggered by player interaction with entities.

If the interaction would open any GUI for a secondary player, the interaction is "queued" until that player becomes primary or cleared upon disconnection.

---

#### `Json` player.shipUpgrades()

Returns a JSON object containing information about the player's current ship upgrades including "shipLevel", "maxFuel", "crewSize" and a list of "capabilities".

---

#### `void` player.upgradeShip(`Json` shipUpgrades)

Applies the specified ship upgrades to the player's ship.

---

#### `void` player.setUniverseFlag(`String` flagName)

Sets the specified universe flag on the player's current universe.

---

#### `void` player.giveBlueprint(`ItemDescriptor` item)

Teaches the player any recipes which can be used to craft the specified item.

---

#### `void` player.blueprintKnown(`ItemDescriptor` item)

Returns `true` if the player knows one or more recipes to create the specified item, or `false` otherwise.

---

#### `void` player.makeTechAvailable(`String` tech)

Adds the specified tech to the player's list of available (unlockable) techs.

---

#### `void` player.makeTechUnavailable(`String` tech)

Removes the specified tech from player's list of available (unlockable) techs.

---

#### `void` player.enableTech(`String` tech)

Unlocks the specified tech, allowing it to be equipped through the tech GUI.

---

#### `void` player.equipTech(`String` tech)

Equips the specified tech.

---

#### `void` player.unequipTech(`String` tech)

Unequips the specified tech.

---

#### `JsonArray` player.availableTechs()

Returns a list of the techs currently available to the player.

---

#### `JsonArray` player.enabledTechs()

Returns a list of the techs currently unlocked by the player.

---

#### `String` player.equippedTech(`String` slot)

Returns the name of the tech the player has currently equipped in the specified slot, or `nil` if no tech is equipped in that slot.

---

#### `unsigned` player.currency(`String` currencyName)

Returns the player's current total reserves of the specified currency.

---

#### `void` player.addCurrency(`String` currencyName, `unsigned` amount)

Increases the player's reserve of the specified currency by the specified amount.

---

#### `bool` player.consumeCurrency(`String` currencyName, `unsigned` amount)

Attempts to consume the specified amount of the specified currency and returns `true` if successful and `false` otherwise.

---

#### `void` player.cleanupItems()

Triggers an immediate clean-up of the player's inventory, removing item stacks with 0 quantity. May rarely be required in special cases of making several sequential modifications to the player's inventory within a single tick.

---

#### `void` player.giveItem(`ItemDescriptor` item)

Adds the specified item to the player's inventory. If a `nil` item descriptor is specified, no item is added.

---

#### `bool` player.hasItem(`ItemDescriptor` item, [`bool` exactMatch])

Returns `true` if the player's inventory contains an item matching the specified descriptor and `false` otherwise. If `exactMatch` is `true`, the parameters as well as the item name must match.

---

#### `unsigned` player.hasCountOfItem(`ItemDescriptor` item, [`bool` exactMatch])

Returns the total number of items in the player's inventory matching the specified descriptor. If `exactMatch` is `true`, the parameters as well as the item name must match. Returns `0` if a `nil` descriptor is specified.

---

#### `ItemDescriptor` player.consumeItem(`ItemDescriptor` item, [`bool` consumePartial], [`bool` exactMatch])

Attempts to consume the specified item from the player's inventory and returns the item consumed if successful. If `consumePartial` is `true`, matching stacks totalling fewer items than the requested count may be consumed; otherwise, the operation will only be performed if the full count can be consumed. If `exactMatch` is `true`, the parameters as well as the item name must match. Does nothing if a `nil` descriptor is specified.

---

#### `Map<String, unsigned>` player.inventoryTags()

Returns a summary of all tags of all items in the player's inventory. Keys in the returned map are tag names and their corresponding values are the total count of items including that tag.

---

#### `JsonArray` player.itemsWithTag(`String` tag)

Returns a list of `ItemDescriptor`s for all items in the player's inventory that include the specified tag.

---

#### `void` player.consumeTaggedItem(`String` tag, `unsigned` count)

Consumes items from the player's inventory that include the matching tag, up to the specified count of items.

---

#### `bool` player.hasItemWithParameter(`String` parameter, `Json` value)

Returns `true` if the player's inventory contains at least one item which has the specified parameter set to the specified value.

---

#### `void` player.consumeItemWithParameter(`String` parameter, `Json` value, `unsigned` count)

Consumes items from the player's inventory that have the specified parameter set to the specified value, upt to the specified count of items.

---

#### `ItemDescriptor` player.getItemWithParameter(`String` parameter, `Json` value)

Returns the first item in the player's inventory that has the specified parameter set to the specified value, or `nil` if no such item is found.

---

#### `ItemDescriptor` player.primaryHandItem()

Returns the player's currently equipped primary hand item, or `nil` if no item is equipped.

---

#### `ItemDescriptor` player.altHandItem()

Returns the player's currently equipped alt hand item, or `nil` if no item is equipped.

---

#### `JsonArray` player.primaryHandItemTags()

Returns a list of the tags on the currently equipped primary hand item, or `nil` if no item is equipped.

---

#### `JsonArray` player.altHandItemTags()

Returns a list of the tags on the currently equipped alt hand item, or `nil` if no item is equipped.

---

#### `ItemDescriptor` player.essentialItem(`EssentialItem` slot)

Returns the contents of the specified essential slot, or `nil` if the slot is empty. An `EssentialItem` is one of the following strings:

- `"beamaxe"`
- `"wiretool"`
- `"painttool"`
- `"inspectiontool"`

---

#### `void` player.giveEssentialItem(`EssentialItem` slot, `ItemDescriptor` item)

Sets the contents of the specified essential slot to the specified item; if the descriptor is `nil`, clears the slot. See above for valid `EssentialItem` values.

---

#### `void` player.removeEssentialItem(`EssentialItem` slotName)

Removes the essential item in the specified slot. See above for valid `EssentialItem` values.

---

#### `ItemDescriptor` player.equippedItem(`EquipmentSlot` slot)

Returns the contents of the specified equipment slot, or `nil` if the slot is empty. An `EquipmentSlot` is one of the following strings:

- `"head"`
- `"chest"`
- `"legs"`
- `"back"`
- `"headCosmetic"`
- `"chestCosmetic"`
- `"legsCosmetic"`
- `"backCosmetic"`

---

#### `void` player.setEquippedItem(`String` slotName, `ItemDescriptor` item)

Sets the item in the specified equipment slot to the specified item; will clear the slot if the specified item descriptor is `nil`. See above for valid `EquipmentSlot` values.

---

#### `ItemDescriptor` player.swapSlotItem()

Returns the contents of the player's swap (cursor) slot, or `nil` if the slot is empty.

---

#### `void` player.setSwapSlotItem(`ItemDescriptor` item)

Sets the item in the player's swap (cursor) slot to the specified item, or clears the slot if `nil` is specified.

---

#### `bool` player.canStartQuest(`Json` questDescriptor)

Returns `true` if the player meets all of the prerequisites to start the specified quest and `false` otherwise.

---

#### `QuestId` player.startQuest(`Json` questDescriptor, [`String` serverUuid], [`String` worldId])

Starts the specified quest, optionally using the specified server UUID and world ID, and returns the quest ID of the started quest.

---

#### `bool` player.hasQuest(`String` questId)

Returns `true` if the player has a quest, in any state, with the specified quest ID and `false` otherwise.

---

#### `bool` player.hasAcceptedQuest(`String` questId)

Returns `true` if the player has accepted a quest (which may be active, completed, or failed) with the specified quest ID and `false` otherwise.

---

#### `bool` player.hasActiveQuest(`String` questId)

Returns `true` if the player has a currently active quest with the specified quest ID and `false` otherwise.

---

#### `bool` player.hasCompletedQuest(`String` questId)

Returns `true` if the player has a completed quest with the specified quest ID and `false` otherwise.

---

#### `Maybe<WorldId>` player.currentQuestWorld()

If the player's currently tracked quest has an associated world, returns the ID of that world.

---

#### `List<pair<WorldId, bool>>` player.questWorlds()

Returns a list of world IDs for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked.

---

#### `Maybe<Json>` player.currentQuestLocation()

If the player's currently tracked quest has an associated location (CelestialCoordinate, system orbit, UUID, or system position) returns that location.

---

#### `List<pair<Json, bool>>` player.questLocations()

Returns a list of locations for worlds relevant to the player's current quests, along with a boolean indicating whether that quest is tracked.

---

#### `void` player.enableMission(`String` missionName)

Adds the specified mission to the player's list of available missions.

---

#### `void` player.completeMission(`String` missionName)

Adds the specified mission to the player's list of completed missions.

---

#### `void` player.hasCompletedMission(`String` missionName)

Returns whether the player has completed the specified mission.

---

#### `void` player.radioMessage(`Json` messageConfig, [`float` delay])

Triggers the specified radio message for the player, either immediately or with the specified delay.

---

#### `String` player.worldId()

Returns a `String` representation of the world ID of the player's current world.

---

#### `String` player.serverUuid()

Returns a `String` representation of the player's UUID on the server.

---

#### `String` player.ownShipWorldId()

Returns a `String` representation of the world ID of the player's ship world.

---

#### `bool` player.lounge(`EntityId` loungeableId, [`unsigned` anchorIndex])

Triggers the player to lounge in the specified loungeable entity at the specified lounge anchor index (default is 0).

---

#### `bool` player.isLounging()

Returns `true` if the player is currently occupying a loungeable entity and `false` otherwise.

---

#### `EntityId` player.loungingIn()

If the player is currently lounging, returns the entity ID of what the player is currently lounging in.

---

#### `bool` player.stopLounging()

Makes the player stop lounging.

---

#### `double` player.playTime()

Returns the total played time for the player.

---

#### `bool` player.introComplete()

Returns `true` if the player is marked as having completed the intro instance and `false` otherwise.

---

#### `void` player.setIntroComplete(`bool` complete)

Sets whether the player is marked as having completed the intro instance.

---

#### `void` player.warp(`String` warpAction, [`String` animation], [`bool` deploy])

Immediately warps the player to the specified warp target, optionally using the specified warp animation and deployment.

Only warps the _primary_ player immediately. Any attempt to warp a secondary player will be "queued" until that player becomes primary or cleared upon disconnection.

---

#### `bool` player.canDeploy()

Returns whether the player has a deployable mech.

---

#### `bool` player.isDeployed()

Returns whether the player is currently deployed.

---

#### `RpcPromise` player.confirm(`Json` dialogConfig)

Displays a confirmation dialog to the player with the specified dialog configuration and returns an `RpcPromise` which can be used to retrieve the player's response to that dialog.

---

#### `void` player.playCinematic(`Json` cinematic, [`bool` unique])

Triggers the specified cinematic to be displayed for the player. If `unique` is `true`, the cinematic will only be shown to that player once.

If invoked on a secondary player, the cinematic is "queued" until that player becomes primary or cleared upon disconnection.

---

#### `void` player.recordEvent(`String` event, `Json` fields)

Triggers the specified event on the player with the specified fields. Used to record data e.g. for achievements.

---

#### `bool` player.worldHasOrbitBookmark(`Json` coordinate)

Returns whether the player has a bookmark for the specified celestial coordinate.

---

#### `List<pair<Vec3I, Json>>` player.orbitBookmarks()

Returns a list of orbit bookmarks with their system coordinates.

---

#### `List<Json>` player.systemBookmarks(`Json` systemCoordinate)

Returns a list of orbit bookmarks in the specified system.

---

#### `void` player.addOrbitBookmark(`Json` systemCoordinate, `Json` bookmarkConfig)

Adds the specified bookmark to the player's bookmark list.

---

#### `void` player.removeOrbitBookmark(`Json` systemCoordinate, `Json` bookmarkConfig)

Removes the specified bookmark from the player's bookmark list.

---

#### `List<Json>` player.teleportBookmarks()

Lists all of the player's teleport bookmarks.

---

#### `void` player.addTeleportBookmark(`Json` bookmarkConfig)

Adds the specified bookmark to the player's bookmark list.

---

#### `void` player.removeTeleportBoookmark(`Json` bookmarkConfig)

Removes the specified teleport bookmark.

---

#### `bool` player.isMapped(`Json` coordinate)

Returns whether the player has previously visited the specified coordinate.

---

#### `Json` player.mappedObjects(`Json` systemCoordinate)

Returns uuid, type, and orbits for all system objects in the specified system;

---

#### `List<String>` player.collectables(`String` collectionName)

Returns a list of names of the collectables the player has unlocked in the specified collection.

---

## Identity callbacks

The following callbacks can be used to get or alter the player's current humanoid identity. These are available on xStarbound, OpenStarbound and StarExtensions, but only xStarbound has the full complement described below.

---

#### `void` player.setIdentity(`Json` newIdentity)

#### `void` player.setHumanoidIdentity(`Json` newIdentity)

Sets the player's identity. The new identity will be merged with the current one; as a special case, if the `"imagePath"` key (and _only_ that key) has been explicitly set to a `nil` value, _and_ either the table was created with `jobject()` or its metatable's `"__nils"` table otherwise contains `"imagePath"` (e.g., the metatable is `{["__nils"] = {imagePath = 0}}`), the `"imagePath"` will be set to `null`. Will log a warning and leave the species and/or image path unchanged if the new identity includes a `"species"` and/or `"imagePath"` that doesn't exist in the loaded assets.

---

#### `Json` player.identity()

#### `Json` player.getIdentity()

#### `Json` player.humanoidIdentity()

Gets the player's identity.

---

In addition to the identity callbacks above, the following getters and setters may be used to get and set various items in the player's humanoid identity.

**Notes:** A few notes:

- Any `nil` or unspecified value is ignored, _unless_ it's an image path, in which case the image path is cleared.
- Invalid species names, image paths and genders — for reference, valid genders are `"male"` and `"female"` — are ignored and will log a warning (for species or image paths) or throw an error (for genders).
- The "favourite colour" is the one used for the beams and highlights of "beam" items like the matter manipulator. The beams, but not the highlights, are affected by the alpha (fourth) value.

**Personality:** A `Personality` table looks like this:

```lua
local personality = jobject{
    idle = "idle.1", -- The idle pose for the body.
    armIdle = "idle.1", -- The idle pose for the arms.
    headOffset = jarray{0.0, 0.0}, -- A `Vec2F` in world pixels.
    armOffset = jarray{0.0, 0.0} -- A `Vec2F` in world pixels.
}
```

When passing a personality to `player.setPersonality`, any omitted keys are left unchanged.

### Getters

The following humanoid identity getters are available:

#### `String` player.bodyDirectives()

#### `String` player.emoteDirectives()

#### `String` player.hairGroup()

#### `String` player.hairType()

#### `String` player.hairDirectives()

#### `String` player.facialHairGroup()

#### `String` player.facialHairType()

#### `String` player.facialHairDirectives()

#### `String` player.facialMaskGroup()

#### `String` player.facialMaskType()

#### `String` player.facialMaskDirectives()

#### `String, String, String` player.hair()

#### `String, String, String` player.facialHair()

#### `String, String, String` player.facialMask()

#### `String` player.name()

#### `String` player.species()

#### `String` player.imagePath()

#### `String` player.gender()

#### `Personality` player.personality()

#### `Vec4B` player.favoriteColor()

**Note:** `player.hair`, `player.facialHair` and `player.facialMask` return their items in "group, type, directives" order.

### Setters

The following humanoid identity setters are available:

#### `void` player.setBodyDirectives(`String` newBodyDirectives)

#### `void` player.setEmoteDirectives(`String` newEmoteDirectives)

#### `void` player.setHairGroup(`String` newHairGroup)

#### `void` player.setHairType(`String` newHairType)

#### `void` player.setHairDirectives(`String` newHairDirectives)

#### `void` player.setFacialHairGroup(`String` newFacialHairGroup)

#### `void` player.setFacialHairType(`String` newFacialHairType)

#### `void` player.setFacialHairDirectives(`String` newFacialHairDirectives)

#### `void` player.setFacialMaskGroup(`String` newFacialMaskGroup)

#### `void` player.setFacialMaskType(`String` newFacialMaskType)

#### `void` player.setFacialMaskDirectives(`String` newFacialMaskDirectives)

#### `void` player.setHair(`Maybe<String>` group, `Maybe<String>` type, `Maybe<String>` directives)

#### `void` player.setFacialHair(`Maybe<String>` group, `Maybe<String>` type, `Maybe<String>` directives)

#### `void` player.setFacialMask(`Maybe<String>` group, `Maybe<String>` type, `Maybe<String>` directives)

#### `void` player.setName(`String` name)

#### `void` player.setSpecies(`String` species)

#### `void` player.setImagePath(`Maybe<String>` imagePath)

#### `void` player.setGender(`String` gender)

#### `void` player.setPersonality(`Personality` personalityConfig)

#### `void` player.setFavoriteColor(`Vec4B` favouriteColour)

---

## xStarbound callbacks

The following `player` callbacks are available on xStarbound but _not_ on stock Starbound. Many of these are available with StarExtensions or OpenStarbound.

---

#### `PlayerMode` player.mode()

Returns the player's difficulty mode. Valid `PlayerMode` values are:

- `"casual"`
- `"survival"`
- `"hardcore"`

---

#### `void` player.setMode(`PlayerMode` mode)

Sets the player's difficulty mode. See above for valid `PlayerMode` values.

---

#### `String` player.description()

Returns the player's description. This description can be viewed by xClient, OpenStarbound and StarExtensions users using an inspection tool on the player, and may also be viewed with certain modded items. A new description won't get networked until the player warps or dies.

---

#### `void` player.setDescription(`String` newMessage)

Sets the player's description. This description can be viewed by xClient, OpenStarbound and StarExtensions users using an inspection tool on the player, and may also be viewed with certain modded items. A new description won't get networked until the player warps or dies.

---

#### `float` player.interactRadius()

Returns the player's current interaction radius in world tile lengths.

---

#### `void` player.setInteractRadius(`float` newRadius)

Sets the player's interaction radius in world tile lengths.

---

#### `void` player.queueStatusMessage(`String` newMessage)

Queues a status message, visible at the bottom of the screen. Status messages queued with this callback on a secondary player will not be shown until that player becomes primary, and will be cleared upon disconnection.

Also see `interface.queueMessage` in `interface.md`.

---

#### `void` player.say(`String` newMessage, `Maybe<String>` portrait, `Maybe<EntityId>` sourceEntityId, `Maybe<Json>` bubbleConfig)

#### `void` player.addChatBubble(`String` newMessage, `Maybe<String>` portrait, `Maybe<EntityId>` sourceEntityId, `Maybe<Json>` bubbleConfig)

Spawns a networked chat bubble. Parameters:

- `sourceEntityId`: If not `nil`, this overrides the default source entity ID (which is the player's ID) for the chat bubble. May be used to "throw" chat bubbles like a ventriloquist.
- `portrait`: If `nil`, the chat bubble will be a non-portrait bubble. If a string is provided, the bubble is a portrait bubble; the string is a portrait drawable string.
- `bubbleConfig`: If not `nil`, is an optional bubble config in the form of a JSON map, with parameters optionally specified as follows:
  - `drawMoreIndicator` (`bool`): Portrait bubbles only. If `true`, adds a 'more after this' indicator to a portrait bubble.
  - `sound` (`String`): If present, plays the specified sound asset when the chat bubble is spawned.
  - `drawBorder` (`bool`): Non-portrait bubbles only. If `true`, draws a normal chat bubble; if `false`, renders the chat bubble invisible and may be visually borked. Defaults to `true`.
  - `fontSize` (`uint64_t`): Non-portrait bubbles only. The font size of the text in the chat bubble (or "bubble"). Defaults to the `"fontSize"` in `/interface/windowconfig/chatbubbles.config` (`8` in vanilla).
  - `color` (`Color`): Non-portrait bubbles only. The base colour of the text (before any modifications by colour codes) in the chat bubble. Defaults to the `"textColor"` in `/interface/windowconfig/chatbubbles.config` (`"white"` in vanilla).

The chat bubble is spawned over the player this callback is invoked on, no matter whether the player is primary or secondary, unless a different `sourceEntityId` is specified.

---

#### `void` player.setChatBubbleConfig(`Maybe<Json>` bubbleConfig)

Sets the player's chat bubble config. This affects all chat bubbles the player normally spawns (excluding those spawned with `world.addChatBubble()`). The config is stored in the player file as `"chatConfig"`. JSON parameters:

- `bubbleConfig`: If not `nil`, is an optional bubble config in the form of a JSON map, with parameters optionally specified as follows:
  - `drawMoreIndicator` (`bool`): Portrait bubbles only. If `true`, adds a 'more after this' indicator to a portrait bubble.
  - `portrait`: If `nil`, the chat bubble will be a non-portrait bubble. If a string is provided, the bubble is a portrait bubble; the string is a portrait drawable string.
  - `sound` (`String`): If present, plays the specified sound asset when the chat bubble is spawned.
  - `drawBorder` (`bool`): Non-portrait bubbles only. If `true`, draws a normal chat bubble; if `false`, renders the chat bubble invisible. Defaults to `true`.
  - `fontSize` (`uint64_t`): Non-portrait bubbles only. The font size of the text in the chat bubble (or "bubble"). Defaults to the `"fontSize"` in `/interface/windowconfig/chatbubbles.config` (`8` in vanilla).
  - `color` (`Color`): Non-portrait bubbles only. The base colour of the text (before any modifications by colour codes) in the chat bubble. Defaults to the `"textColor"` in `/interface/windowconfig/chatbubbles.config` (`"white"` in vanilla).

---

#### `Json` player.getChatBubbleConfig()

Gets the player's chat bubble config. This affects all chat bubbles the player normally spawns (excluding those spawned with `player.addChatBubble()`). The config is stored in the player file as `"chatConfig"`.

---

#### `void` player.sendChat(`String` text, `String` sendMode, `Maybe<bool>` addBubble, `Maybe<JsonObject>` metadata)

> _Note:_ Sending, transporting and receiving chat metadata requires xStarbound v3.5.3+ on _both_ the sending and receiving clients _and_ on the server, _all_ running in xStarbound networking mode (not legacy mode!). Due to network compatibility issues, sending, transporting and receiving OpenStarbound v0.1.9+ chat metadata are _not_ supported by xStarbound! (An xStarbound server or host _must_ be running in legacy mode to accept connections from OpenStarbound clients without errors.)

> _Note:_ The chat bubble boolean's functionality was inverted in xStarbound v3.5.3 to avoid a minor annoyance when writing cross-client scripts.

Sends a chat message, _skipping_ client-side command processing. Arguments are as follows:

- `text`: The text to send.
- `sendMode`: If specified, may be any one of `"Local"`, `"Broadcast"` or `"Party"` (anything else resolves to `"Local"`). Defaults to `"Local"`.
- `addBubble`: If explicitly `false`, no chat bubble is spawned when the chat message is sent.

The message will be sent immediately regardless of whether the player is primary or secondary, but the chat bubble (if any) will always spawn above the _primary_ player.

Identical in functionality to `chat.send` (see `interface.md`).

---

#### `void` player.addEffectEmitters(`Json` effectEmitters)

Adds a set of effect emitters to the player this tick. The argument is an array of strings (`effectSource` types).

---

#### `void` player.dropEverything()

Forces the player to drop all inventory items. Don't know why anyone would ever want to use this though.

If this callback is called in any held item's script, `init` or `update` will finish running and `uninit` will run, but item drops will not have any state saved by `item` callbacks after this callback is invoked.

---

#### `Emote` player.currentEmote()

Returns the player's current emote state. Valid `Emote` values are:

- `"Idle"` - No ongoing emote.
- `"Blabbering"` - Talking.
- `"Shouting"`
- `"Happy"`
- `"Sad"`
- `"NEUTRAL"` - You filthy neutral!
- `"Laugh"`
- `"Annoyed"`
- `"Oh"`
- `"OOOH"`
- `"Blink"`
- `"Wink"`
- `"Eat"`
- `"Sleep"`

---

#### `void` player.emote(`Emote` emote, `Maybe<float>` cooldown, `Maybe<bool>` gentleRequest)

Makes the player show the requested emote. See above for valid `Emote` values. If a cooldown is specified, this overrides the default emote cooldown specified under `"emoteCooldown"` in `$assets/player.config`. Any unrecognised emote is assumed `"Idle"`. If `gentleRequest` is `true`, ongoing emotes that aren't `"Idle"` or `"Blink"` aren't "clobbered". Note that emote names are case-sensitive!

---

#### `void` player.setExternalWarpsIgnored(`bool` ignored)

Sets whether external warp requests are ignored. If `true`, external warp requests from servers and other players are ignored; this also removes all other restrictions on teleporting (except the inability to beam down from a ship not orbiting anything, simply because there isn't anything to beam down to). This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalWarpRequests"`.

---

#### `void` player.setExternalRadioMessagesIgnored(`bool` ignored)

Despite the callback name, sets whether external radio, `recordEvent`, `addCollectable` and Lua-handled messages are ignored. If `true`, such external messages from servers and other players are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalRadioMessages"`.

---

#### `void` player.setExternalCinematicsIgnored(`bool` ignored)

Despite the callback name, sets whether external cinematic and alt music messages are ignored; this also sets whether death cinematics are completely skipped. If `true`, external cinematic messages from servers and other players are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalCinematics"`.

---

#### `void` player.setPhysicsEntitiesIgnored(`bool` ignored)

Sets whether all physics entities and force regions are ignored by the player. If `true`, all external physics entities and force regions that aren't object or tile geometry are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreAllPhysicsEntities"`.

---

#### `void` player.setNudityIgnored(`bool` ignored)

Sets whether nudity caused by lounging, status effects, etc., is ignored. If `true`, the player always remains modest (unless the player explicitly undresses). This setting is saved in the player file under `"xSbProperties"` as `"ignoreNudity"`.

---

#### `void` player.setTechOverridesIgnored(`bool` ignored)

Sets whether tech overrides are ignored. If `true`, automatic tech overrides on worlds like the Outpost's tech challenge instances are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreTechOverrides"`.

---

#### `void` player.toggleOverreach(`bool` toggle)

Sets whether "overreach" is enabled. If `true`, the player can interact with any object, vehicle, NPC or monster on screen, even otherwise non-interactive ones. Line-of-sight restrictions are also ignored. This setting is saved in the player file under `"xSbProperties"` as `"overreach"`.

---

#### `void` player.toggleInWorldRespawn(`bool` toggle)

Sets whether in-world respawning is enabled. If `true`, the player respawns on the world where he or she died. This setting is saved in the player file under `"xSbProperties"` as `"inWorldRespawn"`.

---

#### `void` player.setIgnoreItemPickups(`bool` ignore)

Sets whether item pickups are ignored. If `true`, the player will not pick up any items, even those spawned with `player.giveItem()`; any spawned items will instead be dropped next to the player. This setting is saved in the player file under `"xSbProperties"` as `"ignoreItemPickups"`.

---

#### `void` player.setIgnoreShipUpdates(`bool` ignore)

Sets whether shipworld updates are ignored. If `true`, all updates to the player's shipworld are ignored; this even protects against `removeEntity` and other similar modifications. Note that storing items in containers on a protected shipworld will cause them to be lost, and removing items already stored in containers will duplicate them. This setting is saved in the player file under `"xSbProperties"` as `"ignoreShipUpdates"`.

---

#### `void` player.toggleFastWarp(`bool` toggle)

Sets whether "fast warping" is enabled. If `true`, all teleports and warps are instantaneous, and the player has no beaming animation (although one may be supplied in a Lua script). This setting is saved in the player file under `"xSbProperties"` as `"fastWarp"`.

---

#### `bool` player.externalWarpsIgnored()

Gets whether external warp requests from servers and other players are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalWarpRequests"`.

---

#### `bool` player.externalRadioMessagesIgnored()

Gets whether external radio, `recordEvent`, `addCollectable`, and Lua-handled messages from servers and other players are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalRadioMessages"`.

---

#### `bool` player.externalCinematicsIgnored()

Gets whether external cinematic and alt music messages are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreExternalCinematics"`.

---

#### `bool` player.physicsEntitiesIgnored()

Gets whether physics entities and force regions are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreAllPhysicsEntities"`.

---

#### `bool` player.nudityIgnored()

Gets whether nudity caused by lounging, status effects, etc., is ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreNudity"`.

---

#### `bool` player.overreach()

Gets whether "overreach" is enabled. This setting is saved in the player file under `"xSbProperties"` as `"overreach"`.

---

#### `bool` player.techOverridesIgnored()

Gets whether tech overrides are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreTechOverrides"`.

---

#### `bool` player.itemPickupsIgnored()

Gets whether item pickups are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreItemPickups"`.

---

#### `bool` player.fastWarp()

Gets whether fast warping is enabled. This setting is saved in the player file under `"xSbProperties"` as `"fastWarp"`.

---

#### `bool` player.inWorldRespawn()

Gets whether in-world respawning is enabled. This setting is saved in the player file under `"xSbProperties"` as `"inWorldRespawn"`.

---

#### `bool` player.shipUpdatesIgnored()

Gets whether shipworld updates are ignored. This setting is saved in the player file under `"xSbProperties"` as `"ignoreShipUpdates"`.

---

#### `String` player.getChatText()

Gets any text currently in the chat box. Does _not_ return `""` if the chat box isn't focussed but contains any text. Nearly identical to `chat.input` (see `interface.md`).

**Note:** This callback invokes `currentChat` in chat pane scripts (see `interface.md`), so don't have `currentChat` invoke this callback to prevent a recursion error!

---

#### `bool` player.chatHasFocus()

Gets whether the chat box is focussed.

**Note:** This callback invokes `hasFocus` in chat pane scripts (see `interface.md`), so don't have `hasFocus` invoke this callback to prevent a recursion error!

---

#### `void` player.overrideTypingIndicator(`bool` overridden)

If `true`, hides the "is typing" indicator that is displayed above the player's head; if `false`, shows it normally. Useful for when you're replacing it with a custom chat bubble typing indicator or just want to hide it. The override is remembered until the player exits the game or returns to the main menu.

---

#### `void` player.overrideMenuIndicator(`bool` overridden)

If `true`, hides the "in menu" indicator that is displayed above the player's head; if `false`, shows it normally. The override is remembered until the player exits the game or returns to the main menu.

---

#### `Vec2F` player.aimPosition()

Gets the player's aim position.

---

#### `void` player.setDamageTeam(`Maybe<String>` teamType, `Maybe<uint16_t>` teamNumber)

Sets the player's damage team, overriding any server requests. `teamType` may be any of the following:

- `"null"`
- `"friendly"`
- `"enemy"`
- `"pvp"`
- `"passive"`
- `"ghostly"`
- `"environment"`
- `"indiscriminate"`
- `"assistant"`

... and is case-sensitive, defaulting to `"friendly"` if an invalid type is specified. `teamNumber` may be any positive integer or `0`. Any unspecified parameters are set to the default for players - `"friendly"` and `0`, respectively. If _no_ parameters are specified, the player's damage team is returned to whatever the server requests it to be.

To _get_ the player's current damage team, use `entity.team`.

**Note:** Unlike OpenStarbound's equivalent, xClient's changes to the player damage team are persistent. The team config is saved under `"team"` in the player save, while whether the team config has been overridden by xClient is saved under `"damageTeamOverridden"` in the player's `"xSbProperties"`.

---

#### `void` player.setOverrideState(`Maybe<String>` newState)

Overrides the player's humanoid state. Available states are:

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

Any other string will set the state to `"idle"`. If `nil` or no argument is passed, any existing override is cleared. Most tech parent state equivalences are obvious, but note that `"idle"` is equivalent to the tech parent state `"Stand"` and `"jump"` is equivalent to the tech parent state `"Fly"`.

---

#### `void` player.overrideCameraPosition(`Maybe<Vec2F>` newCameraPosition)

Overrides the player client's camera position. Must be applied every tick like `mcontroller.controlParameters()` to keep the camera position overridden. Unlike `interface.overrideCameraPosition` (see `interface.md`), this override does not prevent the **Camera Look** key from being used and only takes effect if called in a script running on the primary player.

---

#### `unsigned` player.actionBarGroup()

Returns the player's active action bar group. The group index begins at `1`, not `0`.

---

#### `void` player.setActionBarGroup(`unsigned` barId)

Sets the player's active action bar group. The group index begins at `1`, not `0`.

---

#### `Variant<unsigned, EssentialItem>` player.selectedActionBarSlot()

Returns the player's selected action bar slot. The slot index begins at `1`, not `0`. An `EssentialItem` is one of the following strings:

- `"beamaxe"`
- `"wiretool"`
- `"painttool"`
- `"inspectiontool"`

---

#### `void` player.setSelectedActionBarSlot(`Variant<unsigned, EssentialItem>` slot)

Sets the player's selected action bar slot. The slot index begins at `1`, not `0`. See above for valid `EssentialItem` values.

---

#### `InventorySlot` player.actionBarSlotLink(`Variant<int, Vec2I>` slotIndex, `String` hand)

Returns the action bar slot link (as an `InventorySlot`), if any, at the specified action bar group/slot/hand combination.

An `InventorySlot` may be any of the following Lua values:

- `"swap"`: Swap slot.
- `"trash"`: Trash slot.
- `"head"`: Head armour (helmet) slot.
- `"chest"`: Chest armour slot.
- `"legs"`: Legs armour slot.
- `"back"`: Back armour slot.
- `"headCosmetic"`: Head vanity slot.
- `"chestCosmetic"`: Chest vanity slot.
- `"legsCosmetic"`: Legs vanity slot.
- `"backCosmetic"`: Back vanity slot.
- `{bag, slot}`: Inventory slot. `bag` is the `String` name of the inventory bag and `slot` is the `int` slot index (beginning at `0`, not `1`!).
- `nil`: No inventory slot. For `player.actionBarSlotLink` and `player.setActionBarSlotLink`, this means an unlinked action bar slot.

The arguments are specified as follows:

- The `slotIndex` may be either an integer or an array of two integers:
  - If it's an _integer_, the lone integer is the specified slot index of the player's _currently selected_ action bar group.
  - If it's an _array_, the first value is the specified action bar group; the second is the slot index for that group. Group and slot indices begin at `1`, not `0`.
- The hand may be either `"primary"` or `"alt"`.

---

#### `void` player.setActionBarSlotLink(`Variant<int, Vec2I>` slotIndex, `String` hand, `InventorySlot` slotLink)

Sets or clears the action bar slot link (as an `InventorySlot`) at the specified action bar group/slot/hand combination.

The arguments are specified as follows:

- `slotIndex` is as for `player.actionBarSlotLink` above.
- `hand` is as for `player.actionBarSlotLink` above.
- `slotLink` is an inventory slot specifier; see above for valid `InventorySlot` values.

The following caveats apply to item slot links:

- Linking a _two-handed_ item to a _primary_ slot sets _both_ the primary and alt hands at the specified group/slot combination. (Technically, the alternate slot isn't actually set to the primary item, but acts as if it were.)
- Any attempt to link a _two-handed_ item to an _alternate_ slot is ignored.
- Linking a _one-handed_ item to an _alternate_ slot while the _primary_ slot has a _two-handed_ item in it clears the _primary_ slot in addition to filling the _alternate_ slot.
- If the specified item slot has a one-handed item in it _and_ matches the slot linked to the _other_ hand, the links are swapped.
- A `nil` slot link or a link to an empty inventory slot clears any link at the specified group/slot/hand combination.

---

#### `size_t` player.itemBagSize(`String` bagName)

Returns the number of slots in the specified inventory bag.

---

#### `size_t` player.itemAllowedInBag(`String` bagName, `ItemDescriptor` item, `Maybe<bool>` forceCheck)

> _The `forceCheck` parameter is only available on xClient._

Returns whether the specified item is allowed to go in the specified inventory bag. If `forceCheck` is `false` or unspecified _and_ `"allowAnyBagItem"` under `"inventory"` in `$assets/player.config` is `true`, this callback will always return `true`. If `forceCheck` is `true`, the callback will act as if `"allowAnyBagItem"` were `false`, even if it is otherwise enabled by an asset mod.

---

#### `ItemDescriptor` player.item(`InventorySlot` slot)

Returns the item in the specified inventory slot, if any. See `player.actionBarSlotLink` above for valid `InventorySlot` values.

---

#### `bool` player.setItem(`InventorySlot` slot, `ItemDescriptor` item)

Sets the specified inventory slot to the specified item, or clears the slot if the item is `nil`. See `player.actionBarSlotLink` above for valid `InventorySlot` values. Will fail if the specified item is not allowed to go in the specified slot. Returns `true` if successful or `false` otherwise.

---

#### `void` player.setToolUsageSuppressed(`bool` suppressed)

Sets whether the player can use held items. Identical to `tech.setToolUsageSuppressed` (see `tech.md`).

**Note:** The internal values used by _both_ `player.setToolUsageSuppressed` and `tech.setToolUsageSuppressed` must be `false` for the player to use held items.

---

#### `Json` player.teamMembers()

Returns a list of all team members in the player's current team, _including_ the current player. If the player is not currently in any team, returns `jarray{}`. The list has the following format:

```lua
local teamMembers = jarray{
    jobject{
        name = "FezzedOne", -- The team member's current name.
        uuid = "0123456789abcedf0123456789abcedf", -- The team member's current UUID.
        entity = 12345, -- The team member's current entity ID.
        healthPercentage = 1.0, -- The ratio of the team member's current HP to maximum HP.
        energyPercentage = 0.9, -- The ratio of the team member's current energy to maximum energy.
        position = {123.4, 567.8}, -- The team member's current world position.
        world = "InstanceWorld:outpost:-:-", -- The world on which the team member is currently located.
        warpMode = "BeamOrDeploy", -- One of `"None"`, `"BeamOnly"`, `"DeployOnly"` and `"BeamOrDeploy"`.
        portrait = jarray{...} -- The team member's portrait drawables.
    },
    ...
}
```

---

#### `void` player.controlAimPosition(`Vec2F` newAimPosition)

Makes the player aim at the specified world position.

For a primary player, the aim position set by this callback will be reset to the cursor's world position on the next tick unless this callback is called again.

For a secondary player, the aim position will remain set until the primary player warps or dies, the aim position is changed via this callback again, or the secondary player becomes primary.

---

#### `void` player.controlShifting(`Maybe<bool>` shifting)

If `true` is passed, makes the player begin "shifting" exactly as if the **Walk** key (<kbd>Shift</kbd> by default) were held. If `false` or `nil` is passed, or the parameter is unspecified, makes the player stop "shifting".

A primary player's "shifting" state will be reset to the state of the **Walk** key on the next tick.

For a secondary player, the "shifting" state will remain set until the primary player warps or dies, the state is changed via a callback again, or the secondary player becomes primary.

---

#### `void` player.controlTrigger(`Maybe<bool>` triggering)

If `true` is passed, makes the player begin "triggering" whatever is at his or her currently set aim position exactly as if the **Interact** key (<kbd>E</kbd> by default) or middle mouse button were pressed.

If `false` or `nil` is passed, or the parameter is unspecified, makes the player stop "triggering", exactly as if the **Interact** key or the middle mouse button were released.

A primary player's "triggering" state will be reset to the state of the **Walk** key on the next tick.

For a secondary player, the "triggering" state will remain set until the primary player warps or dies, the state is changed via a callback again, or the secondary player becomes primary.

---

#### `void` player.controlFire(`Maybe<String>` fireMode)

If `"primary"`/`"beginPrimary"` or `"alt"`/`"beginAlt"` is passed, makes the player begin primary or alt fire, respectively, exactly as if the left or right mouse button were pressed, respectively.

If `"endPrimary"` or `"endAlt"` is passed, makes the player end primary or alt fire, respectively, exactly as if the left or right mouse button were released, respectively.

If `nil` is passed, or the parameter is unspecified, makes the player end primary _and_ alt fire immediately, exactly as if both mouse buttons were released.

A primary player's primary/alt firing state will be reset to the state of the mouse buttons on the next tick.

For a secondary player, the primary/alt firing state will remain set until the primary player warps or dies, the state is changed via a callback again, or the secondary player becomes primary.

The `fireMode` parameter is case-sensitive.

---

#### `void` player.controlSpecialAction(`int` specialAction)

Makes the player activate the specified special action — one of `1`, `2` or `3` — this tick, exactly as if the **Special 1**, **2** or **3** key were pressed, respectively.

**Note:** The **Special 1** keybind is bound to <kbd>F</kbd> by default, and is used to toggle the vanilla sphere techs. **Special 2** and **3** are unbound by default.

---

#### `void` player.controlAction(`Maybe<String>` action)

Used to make the player perform various special actions, depending on the specified string:

- **`"left"`:** The player moves left this tick.
- **`"right"`:** The player moves right this tick.
- **`"down"`:** The player moves down this tick.
- **`"up"`:** The player moves up this tick.
- **`"jump"`:** The player jumps (or continues jumping) this tick.
- **`"beginTrigger"`:** The player begins a trigger action.
- **`"endTrigger"`:** The player ends a trigger action.
- **`"beginPrimaryFire"`:** The player begins primary fire.
- **`"endPrimaryFire"`:** The player ends primary fire.
- **`"beginAltFire"`:** The player begins alternate fire.
- **`"endAltFire"`:** The player ends alternate fire.
- **`"shift"`:** The player starts "shifting".
- **`"unshift"`:** The player stops "shifting".
- **`"special1"`:** The player activates special action 1 this tick.
- **`"special2"`:** The player activates special action 2 this tick.
- **`"special3"`:** The player activates special action 3 this tick.
- **`"dropItem"`:** The player drops the currently held item, throwing it toward the player's currently set aim position.

The `action` is case-insensitive. If `nil` or any other string is specified, or no parameter is specified, the callback does nothing.

**Note:** If `player.controlAction("dropItem")` is called in the dropped item's script, `init` or `update` will finish running and `uninit` will run, but the item drop will not have any state saved by `item` callbacks after this callback is invoked.
