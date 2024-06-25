# `statistics` and statistics scripts

Statistics scripts are run on the client whenever a player triggers a configured statistics event, if the event or achievement has any scripts configured for it. Each individual event or achievement (not *type*, but *instance*) has its own context.

Note that achievement scripts only get run whenever a stat script ticks up a stat counter enough to trigger the relevant achievement.

----

## Callbacks

The following `statistics` callbacks are available:

#### `void` statistics.setStat(`String` name, `String` type, `Json` value)

Sets the stat with the specified name to the specified string type and JSON value.

#### `Json` statistics.setStat(`String` name, `Json` default)

Returns the JSON data for the stat with the specified name, or if it doesn't exist, the specified default (or `nil` if no default is specified).

#### `Maybe<String>` statistics.statType(`String` name)

Returns the string type of the specified stat, or `nil` if the specified stat doesn't exist.

#### `bool` statistics.achievementUnlocked(`String` name)

Returns whether the specified achievement has been unlocked.

#### `bool` statistics.checkAchievement(`String` name)

Checks whether the specified achievement should be unlocked, running any achievement script if necessary to do the check. Returns whether the specified achievement has been *or* should be unlocked.

#### `void` statistics.unlockAchievement(`String` name)

Unlocks the specified achievement. "Achievement get!"

----

## Invoked functions

#### [`void` init()]
#### [`Maybe<bool>` check(`String` achievementName)]
#### [`void` event(`String` name, `Json` fields)]
#### [`void` uninit()]

These script functions are invoked one after the other on the same tick in the given order when a statistics script is loaded and invoked. The context is then destroyed immediately thereafter.

`check` is only invoked in achievement scripts. The engine expects this function to return a boolean indicating whether the achievement with the specified name has actually been achieved.

`event` is only invoked in event scripts. The engine passes the event's name and an arbitrary JSON value containing fields that the event script is intended to validate before setting statistics using the callbacks below. No returns are expected.

----

## Hardcoded events

The `"trashItem"` and `"item"` events are hardcoded in Starbound (although this may change in the future for xStarbound). `"item"` is triggered whenever a player picks up an item and `"trashItem"` is triggered when an item is destroyed in a player's trash slot. Both have the following JSON fields:

```lua
jobject{
    itemName = "perfectlygenericitem", -- The item's internal name.
    count = 20, -- The count of the item.
    category = "generic" -- The item's `"category"` value.
}
```

Additionally, the `"useItem"` event, triggered upon consuming any consumable item, is hardcoded as a local entity message sent to the entity consuming the item (although, again, this may change in the future for xStarbound), and has the following JSON field:

```lua
jobject{
    itemType = "banana" -- The consumed item's internal name.
}
```

----

All other events may be arbitrarily changed by asset mods. Note that there is a technically hardcoded `"recordEvent"` message that may be arbitrarily handled by script mods (see `message.md`)