# `widget`

The `widget` table contains callbacks used to manipulate and get data about widgets in a scripted pane. This table is available within the following script contexts:

- pane scripts
- container interface scripts

The `widgetName` passed into most of these callbacks can contain period separators for getting children. This is *distinct* from the JSON path syntax explained in `root.md`.

Example:
```lua
widget.getPosition("itemScrollArea.itemList.1.name")
```

**Note:** All pixel sizes are in *interface* pixels, so there is rarely any need to account for changes to interface scale.

## General callbacks

These callbacks are available for all widgets.

---

#### `void` widget.playSound(`String` audio, [`int` loops = 0], [`float` volume = 1.0f])

Plays a sound.

---

#### `Vec2I` widget.getPosition(`String` widgetName)

Returns the position of a widget.

---

#### `void` widget.setPosition(`String` widgetName, `Vec2I` position)

Sets the position of a widget.

---

#### `Vec2I` widget.getSize(`String` widgetName)

Returns the size of a widget.

---

#### `void` widget.setSize(`String` widgetName, `Vec2I` size)

Sets the size of a widget.

---

#### `void` widget.setVisible(`String` widgetName, `bool` visible)

Sets the visibility of a widget.

---

#### `void` widget.active(`String` widgetName)

Returns whether the widget is visible.

---

#### `void` widget.focus(`String` widgetName)

Sets focus on the specified widget.

---

#### `void` widget.hasFocus(`String` widgetName)

Returns whether the specified widget is currently focused.

---

#### `void` widget.blur(`String` widgetName)

Unsets focus on the specified focused widget.

---

#### `Json` widget.getData(`String` widgetName)

Returns the arbitrary data value set for the widget.

---

#### `void` widget.setData(`String` widgetName, `Json` data)

Sets arbitrary data for the widget.
---

#### `String` widget.getChildAt(`Vec2I` screenPosition)

Returns the full name for any child widget at `screenPosition`.

---

#### `bool` widget.inMember(`String` widgetName, `Vec2I` screenPosition)

Returns whether the widget contains the specified screenPosition.

---

#### `void` widget.addChild(`String` widgetName, `Json` childConfig, [`String` childName])

Creates a new child widget with the specified config and adds it to the specified widget, optionally with the specified name.

---

#### `void` widget.removeAllChildren(`String` widgetName)

Removes all child widgets of the specified widget.

---

#### `void` widget.removeChild(`String` widgetName, `String` childName)

Removes the specified child widget from the specified widget.

---

## Widget-specific callbacks

These callbacks only work for some widget types.

---

#### `String` widget.getText(`String` widgetName)

Returns the text set in a `TextBoxWidget`.

---

#### `void` widget.setText(`String` widgetName, `String` text)

Sets the text of a `LabelWidget`, `ButtonWidget` or `TextBoxWidget`.

---

#### `void` widget.setFontColor(`String` widgetName, `Color` color)

Sets the font color of a `LabelWidget`, `ButtonWidget` or `TextBoxWidget`.

---

#### `void` widget.setImage(`String` widgetName, `String` imagePath)

Sets the image of an `ImageWidget`.

---

#### `void` widget.setImageScale(`String` widgetName, `float` imageScale)

Sets the scale of an `ImageWidget`.

---

#### `void` widget.setImageRotation(`String` widgetName, `float` imageRotation)

Sets the rotation of an `ImageWidget`.

---

#### `void` widget.setButtonEnabled(`String` widgetName, `bool` enabled)

Sets whether the `ButtonWidget` should be enabled.

---

#### `void` widget.setButtonImage(`String` widgetName, `String` baseImage)

Sets the baseImage of a `ButtonWidget`.

---

#### `void` widget.setButtonImages(`String` widgetName, `Json` imageSet)

Sets the full image set of a `ButtonWidget`. Example `imageSet`:

```lua
jobject{
  base = "image.png",
  hover = "image.png",
  pressed = "image.png",
  disabled = "image.png",
}
```

---

#### `void` widget.setButtonCheckedImages(`String` widgetName, `Json` imageSet)

Similar to `widget.setButtonImages`, but sets the images used for the checked state of a checkable `ButtonWidget`.

---

#### `void` widget.setButtonOverlayImage(`String` widgetName, `String` overlayImage)

Sets the overlay image of a `ButtonWidget`.

---

#### `bool` widget.getChecked(`String` widgetName)

Returns whether the `ButtonWidget` is checked.

---

#### `void` widget.setChecked(`String` widgetName, `bool` checked)

Sets whether a `ButtonWidget` is checked

---

#### `int` widget.getSelectedOption(`String` widgetName)

Returns the index of the selected option in a `ButtonGroupWidget`.

---

#### `int` widget.getSelectedData(`String` widgetName)

Returns the data of the selected option in a `ButtonGroupWidget`, or `nil` if no option is selected.

---

#### `void` widget.setSelectedOption(`String` widgetName, `int` index)

Sets the selected option index of a `ButtonGroupWidget`.

---

#### `void` widget.setOptionEnabled(`String` widgetName, `int` index, `bool` enabled)

Sets whether a `ButtonGroupWidget` option should be enabled.

---

#### `void` widget.setOptionVisible(`String` widgetName, `int` index, `bool`, visible)

Sets whether a `ButtonGroupWidget` option should be visible.

---

#### `void` widget.setProgress(`String` widgetName, `float` value)

Sets the progress percentage of a `ProgressWidget`. Value should be between `0.0` and `1.0`.

---

#### `void` widget.setSliderEnabled(`String` widgetName, `bool` enabled)

Sets whether the `SliderBarWidget` should be enabled.

---

#### `float` widget.getSliderValue(`String` widgetName)

Gets the current value of a `SliderBarWidget`.

---

#### `void` widget.setSliderValue(`String` widgetName, `int` newValue)

Sets the current value of a `SliderBarWidget`.

---

#### `void` widget.getSliderRange(`String` widgetName, `int` newMin, `int` newMax, [`int` newDelta])

Sets the minimum, maximum and (optionally) delta values of a `SliderBarWidget`.

---

#### `void` widget.clearListItems(`String` widgetName)

Clears all items in a `ListWidget`.

---

#### `String` widget.addListItem(`String` widgetName)

Adds a list item to a `ListWidget` using the configured template, and returns the name of the added list item.

---

#### `void` widget.removeListItem(`String` widgetName, `size_t` at)

Removes a list item from a `ListWidget` at a specific index.

---

#### `String` widget.getListSelected(`String` widgetName)

Returns the name of the currently selected widget in a `ListWidget`.

---

#### `void` widget.setListSelected(`String` widgetName, `String` selected)

Sets the selected widget of a `ListWidget`.

---

#### `void` widget.registerMemberCallback(`String` widgetName, `String` callbackName, `LuaFunction` callback)

Registers a member callback for a `ListWidget`'s list items to use.

---

#### `ItemBag` widget.itemGridItems(`String` widgetName)

Returns the full item bag contents of an `ItemGridWidget`.

---

#### `ItemDescriptor` widget.itemSlotItem(`String` widgetName)

Returns the descriptor of the item in the specified item slot widget.

---

#### `void` widget.setItemSlotItem(`String` widgetName, `Json` itemDescriptor)

Sets the item in the specified item slot widget.

---

#### `void` widget.setItemSlotProgress(`String` widgetName, `float` progress)

Sets the progress overlay on the item slot to the specified value (between 0 and 1).

---

#### `Maybe<String>` widget.getHint(`String` widgetName)

> **Only available on xStarbound v3.4.5.1+ and OpenStarbound v0.1.9+.**

Gets the hint text of a text box widget. Returns `nil` if the widget in question doesn't exist or isn't a text box.

---

#### `void` widget.setHint(`String` widgetName, `String` hint)

> **Only available on xStarbound v3.4.5.1+ and OpenStarbound v0.1.9+.**

Sets the hint text of a text box widget.

---

#### `Maybe<size_t>` widget.getCursorPosition(`String` widgetName)

> **Only available on xStarbound v3.4.5.1+ and OpenStarbound v0.1.9+.**

Gets the cursor position of a text box widget. Returns `nil` if the widget in question doesn't exist or isn't a text box.

---

#### `void` widget.setCursorPosition(`String` widgetName, `int` cursorPosition)

> **Only available on xStarbound v3.4.5.1+ and OpenStarbound v0.1.9+.**

Sets the cursor position of a text box widget.

---

#### `CanvasWidget` widget.bindCanvas(`String` widgetName)

Binds the canvas widget with the specified name as a `CanvasWidget` userdata object for easy access. The `CanvasWidget` has the following methods:

**Note:** If a `CanvasWidget` object is returned from `interface.bindCanvas` with `ignoreInterfaceScale` set to `true`, all specified and returned positional values will be in actual screen pixels; otherwise, they will be in scaled interface pixels.

**Note:** A removed canvas widget object will "dangle" even if it's removed with `pane.removeWidget` (see `scriptpane.md`). As long as you have a `CanvasWidget` object, you can still safely render stuff with it.

----

##### `Vec2I` `[CanvasWidget]`:size()

Returns the size of the canvas.

##### `void` `[CanvasWidget]`:clear()

Clears the canvas.

##### `Vec2I` `[CanvasWidget]`:mousePosition()

Returns the mouse position relative to the canvas. Origin is the canvas's lower left corner.

##### `void` `[CanvasWidget]`:drawDrawable(`Drawable` drawable, [`Vec2F` position])

> **Available only on xStarbound and OpenStarbound.**

Draws an arbitrary drawable to the canvas, optionally with a specified position to use as the origin for any position specified in the drawable itself.

##### `void` `[CanvasWidget]`:drawDrawables(`List<Drawable>` drawables, [`Vec2F` position])

> **Available only on xStarbound and OpenStarbound.**

Draws an arbitrary list of drawables to the canvas, optionally with a specified position to use as the origin for any positions specified in the drawables themselves.

##### `void` `[CanvasWidget]`:drawImage(`String` image, `Vec2F` position, [`float` scale], [`Color` colour], [`bool` centered])

Draws an image to the canvas.

##### `void` `[CanvasWidget]`:drawImageDrawable(`String` image, `Vec2F` position, [`Variant<Vec2F, float>` scale], [`Color` colour], [`float` rotation])

Draws an image to the canvas, centered on `position`, with slightly different options.

##### `void` `[CanvasWidget]`:drawImageRect(`String` texName, `RectF` texCoords, `RectF` screenCoords, [`Color` colour])

Draws a `Rect` section of a texture to a `Rect` section of the canvas.

##### `void` `[CanvasWidget]`:drawTiledImage(`String` image, `Vec2F` offset, `RectF` screenCoords, [`float` scale], [`Color` colour])

Draws an image tiled (and wrapping) within the specified screen area.

##### `void` `[CanvasWidget]`:drawLine(`Vec2F` start, `Vec2F` end, [`Color` colour], [`float` lineWidth])

Draws a line on the canvas.

##### `void` `[CanvasWidget]`:drawRect(`RectF` rect, `Color` colour)

Draws a filled rectangle on the canvas.

##### `void` `CanvasWidget`:drawPoly(`PolyF` poly, `Color` colour, [`float` lineWidth])

Draws a polygon on the canvas.

##### `void` `CanvasWidget`:drawTriangles(`List<PolyF>` triangles, [`Color` colour])

Draws a list of filled triangles to the canvas.

##### `void` `CanvasWidget`:drawText(`String` text, `Json` textPositioning, `unsigned` fontSize, [`Color` colour], [`float` lineSpacing], [`Directives` directives])

Draws text on the canvas. `textPositioning` is in the following format:

```lua
jobject{
  position = jarray{0, 0}
  horizontalAnchor = "left", -- left, mid, right
  verticalAnchor = "top", -- top, mid, bottom
  wrapWidth = nil -- wrap width in pixels or nil
}
```

Use `font` escape codes to specify fonts. See `$doc/directives.md` for information on escape codes.
