# `pane` in non-container panes

These `pane` bindings are available to scripted interface panes and include functions not specifically related to widgets within the pane.

---

#### `EntityId` pane.sourceEntity()

Returns the entity id of the pane's source entity.

---

#### `void` pane.dismiss()

Closes the pane.

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

Gets the pane window's position relative to the upper left corner of the window, in scaled interface pixels. The position is that of the pane's upper left corner.

---

#### `void` pane.setPosition(`Vec2I` newPosition)

Sets the pane window's position relative to the upper left corner of the window, in scaled interface pixels. The position is that of the pane's upper left corner.

---

#### `Vec2I` pane.getSize()

Returns the pane window's size in scaled interface pixels.

---

#### `void` pane.setSize(`Vec2I` newSize)

Sets the pane window's size in scaled interface pixels.

---

#### `void` pane.addWidget(`Json` widgetConfig, [`String` widgetName])

Creates a new widget with the specified config and adds it to the pane, optionally with the specified name.

---

#### `void` pane.removeWidget(`String` widgetName)

Removes the specified widget from the pane.
