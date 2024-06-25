# `item`

The `item` table is available in all item scripts and contains functions relating to the item itself.

---

#### `String` item.name()

Returns the name of the item.

---

#### `size_t` item.count()

Returns the stack count of the item.

---

#### `size_t` item.setCount(`size_t` count)

Sets the item count. Returns any overflow.

---

#### `size_t` item.maxStack()

Returns the max number of this item that will fit in a stack.

---

#### `bool` item.matches(`ItemDescriptor` desc, [`bool` exactMatch])

Returns whether the item matches the specified item. If `exactMatch` is `true` then both the items' names and parameters are compared, otherwise only the items' names.

---

#### `bool` item.consume(`size_t` count)

Consumes items from the stack. Returns whether the full count was successfuly consumed.

---

#### `bool` item.empty()

Returns whether the item stack is empty.

---

#### `ItemDescriptor` item.descriptor()

Returns an item descriptor for the item.

---

#### `String` item.description()

Returns the description for the item.

---

#### `String` item.friendlyName()

Returns the short description for the item.

---

#### `int` item.rarity()

Returns the rarity for the item. An item's rarity determines the colour of the border around the item's icon in an inventory, container, shop, etc. If an `"xSBrarity"` parameter exists, that is used instead of `"rarity"`. The possible rarities are as follows:

- **0**: common (white)
- **1**: uncommon (green)
- **2**: rare (bright blue)
- **3**: legendary (dark purple)
- **4**: essential (yellow)
- **5**: colour 1 (black)
- **6**: colour 2 (grey)
- **7**: colour 3 (dark green)
- **8**: colour 4 (bright green)
- **9**: colour 5 (dark red)
- **10**: colour 6 (bright red)
- **11**: colour 7 (dark orange)
- **12**: colour 8 (bright orange)
- **13**: colour 9 (gold)
- **14**: colour 10 (bright yellow)
- **15**: colour 11 (dark magenta)
- **16**: colour 12 (bright purple)
- **17**: colour 13 (deep blue)
- **18**: colour 14 (fuchsia)
- **19**: colour 15 (bright cyan)
- **20**: colour 16 (dark cyan)

---

#### `String` item.rarityString()

Returns the rarity as a string. An item's rarity determines the colour of the border around the item's icon in an inventory, container, shop, etc. If an `"xSBrarity"` parameter exists, that is used instead of `"rarity"`. The possible rarities are as follows:

- `"common"` (white)
- `"uncommon"` (green)
- `"rare"` (bright blue)
- `"legendary"` (dark purple)
- `"essential"` (yellow)
- `"colour1"` (black)
- `"colour2"` (grey)
- `"colour3"` (dark green)
- `"colour4"` (bright green)
- `"colour5"` (dark red)
- `"colour6"` (bright red)
- `"colour7"` (dark orange)
- `"colour8"` (bright orange)
- `"colour9"` (gold)
- `"colour10"` (bright yellow)
- `"colour11"` (dark magenta)
- `"colour12"` (bright purple)
- `"colour13"` (deep blue)
- `"colour14"` (fuchsia)
- `"colour15"` (bright cyan)
- `"colour16"` (dark cyan)

---

#### `size_t` item.price()

Returns the item's price.

---

#### `unsigned` item.fuelAmount()

Returns the item's fuel amount.

---

#### `Json` item.iconDrawables()

Returns a list of the item's icon drawables.

---

#### `Json` item.dropDrawables()

Returns a list of the item's itemdrop drawables.

---

#### `String` item.largeImage()

Returns the item's configured large image, if any.

---

#### `String` item.tooltipKind()

Returns the item's tooltip kind.

---

#### `String` item.category()

Returns the item's category

---

#### `String` item.pickupSound()

Returns the item's pickup sound.

---

#### `bool` item.twoHanded()

Returns whether the item is two handed.

---

#### `float` item.timeToLive()

Returns the items's time to live.

---

#### `Json` item.learnBlueprintsOnPickup()

Returns a list of the blueprints learned on picking up this item.

---

#### `bool` item.hasItemTag(`String` itemTag)

Returns whether the set of item tags for this item contains the specified tag.

---

#### `Json` item.pickupQuestTemplates()

Returns a list of quests acquired on picking up this item.
