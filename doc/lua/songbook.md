# `songbook`

> **Available only on xStarbound v4.5.3+ and OpenStarbound.**

These bindings are available only in generic player scripts. Use entity messages (see `$docs/lua/message.md`) or Lua «smuggling» (see `$docs/lua.md`) if you need access to them elsewhere.

> **Note:** OpenStarbound and oSBM give NPCs songbooks (and therefore these `songbook` bindings in NPC scripts). oSB-type NPC songbooks are _not_ network-compatible with retail netcode or xStarbound's netcode (and thus not supported by xStarbound or retail Starbound).

---

#### `String` songbook.band()

Returns a string containing the player's current band. Defaults to `""`.

---

#### `Json` songbook.song()

Returns the notes of the song currently being played, in the following format:

```lua
jobject{
    abc = "<ABC notation>"
}
```

... or `nil` if no song is currently being played by the player. The song data may contain other arbitrary JSON data, which can potentially be useful as data storage for script mods. The only required key is `"abc"`, whose value must be a string constituting a (hopefully valid) ABC song.

---

#### `void` songbook.keepAlive(`String` instrument, `Vec2F` sourcePosition)

Keeps the songbook alive this client tick (and for up to two ticks thereafter) using the specified instrument and specified music source position (which should be the player's mouth position, or at least the player's movement controller position).

If the player is not holding an instrument item, this binding must be invoked _every tick_ (or at least once every three ticks) while the player is performing the song; this allows the player to be holding non-instrument items while performing. To swap instruments (or instrument sounds, rather), change the instrument argument.

---

#### `void` songbook.stop()

Causes the player to stop playing any song.

---

#### `bool` songbook.active()

Returns whether the player is playing a song.

---

#### `bool` songbook.instrumentPlaying()

Returns whether the player is currently performing part of a song (i.e., actively playing any notes on an instrument, as opposed to waiting until the song determines the player should play any notes).

---

#### `void` songbook.play(`Json` song)

Causes the player to begin playing the specified song. The JSON data must be either `null` (or `nil`) or of the following format:

```lua
jobject{
  abc = "<ABC notation>"
}
```

Any `nil` or `null` value removes any currently playing song from play. Any invalid values cause an error to be logged.

As long as it is a JSON object, the song data may contain other arbitrary JSON data, which can potentially be useful as data storage for script mods. The only required key is `abc`, whose value must be a string constituting a (hopefully valid) ABC song.

Note that either the player must be holding an instrument item or `songbook.keepAlive` must be invoked by a player script _every tick_ (or at least once every three ticks) as long as the song is playing.

---

#### `String` songbook.instrument()

Returns the type of instrument currently playing or (if no instrument is currently playing) last played, as determined by the `"kind"` in the instrument's config asset. Defaults to `""`.
