# `pane` in non-container panes

These `pane` bindings are available in thend include functions not specifically related to widgets within the pane. They are available in the following script contexts:

- pane scripts (including the **Voice Settings** and **Mod Binds** dialogues on xStarbound and OpenStarbound)
- container interface scripts

Additionally, `interface.bindRegisteredPane` (see `interface.md`) can also return a `LuaCallbacks` table containing the bindings below — replace `pane` with the returned table. The bindings in the returned table will be "bound" to the registered pane specified in the call that returned them.

---

#### `LuaCallbacks` pane.toWidget()

> **Available only on xStarbound and OpenStarbound.**

Returns a table of widget callbacks (see `widget.md`). The returned callbacks are "bound" to *this* pane as a widget.

> **Warning:** If you have `"safeScripts"` disabled on xStarbound, or regardless of this on other clients, do *not* smuggle and use the returned callbacks after the "bound" pane is deregistered or uninitialised! Doing so anyway *will* cause a segfault!

---

#### `EntityId` pane.sourceEntity()

Returns the entity ID of the pane's source entity. *Not* available on registered panes bound with `interface.bindRegisteredPane`.

---

#### `bool` pane.isDisplayed()

> **Available only on xStarbound and OpenStarbound.**

Returns whether the pane is currently displayed. 

This callback is only useful when invoked on registered panes bound with `interface.bindRegisteredPane` — undisplayed script panes aren't running their scripts to begin with, but registered panes are always loaded (while the client is in game) even when they're not displayed.

---

#### `void` pane.dismiss()

Closes the pane. Will invoke the script's `dismiss` function.

---

#### `void` pane.playSound(`String` sound, [`int` loops], [`float` volume])

Plays the specified sound asset, optionally looping the specified number of times or at the specified volume.

---

#### `bool` pane.stopAllSounds(`String` sound)

Stops all instances of the given sound asset, and returns `true` if any sounds were stopped and `false` otherwise.

---

#### `void` pane.setTitle(`String` title, `String` subtitle)

Sets the pane window's title and subtitle.

---

#### `void` pane.setTitleIcon(`String` image)

Sets the pane window's icon.

---

#### `Vec2I` pane.getPosition()

Gets the pane window's position relative to the lower left corner of the window, in scaled interface pixels. The position is that of the pane's lower left corner.

---

#### `void` pane.setPosition(`Vec2I` newPosition)

Sets the pane window's position relative to the lower left corner of the window, in scaled interface pixels. The position is that of the pane's lower left corner.

---

#### `Vec2I` pane.getSize()

Returns the pane window's size in scaled interface pixels.

---

#### `void` pane.setSize(`Vec2I` newSize)

Sets the pane window's size in scaled interface pixels.

---

#### `void` pane.addWidget(`Json` widgetConfig, [`String` widgetName])

> *Returns `LuaCallbacks` for the newly added widget on OpenStarbound.* 

Creates a new widget with the specified config and adds it to the pane, optionally with the specified name. If no name is specified, a random unique name will be generated.

> **Note:** On xStarbound, if you need to access a newly added widget, use `pane.toWidget` and then invoke appropriate callbacks on the newly added sub-widget. This has actual safety checking. Also consider doing this on OpenStarbound to avoid crashes.

> **Warning for OpenStarbound modders:** Invoking these callbacks will cause a segfault when the widget is removed with `pane.removeWidget` or other means, or when they're smuggled outside the context.

---

#### `void` pane.removeWidget(`String` widgetName)

Removes the specified widget from the pane.

---

#### `float` pane.scale()

> **Available only on xStarbound and OpenStarbound. Returns an `int` on OpenStarbound.**

Returns the interface scale. Identical to `interface.scale`.