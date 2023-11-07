# xSB-2

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xSB-2. Base game assets are not provided for obvious reasons.

Compiled builds for Linux and Windows should be available in the usual place on this repository.

If you're compiling xSB-2 anyways, make sure it loads the game assets in `/assets/xSBassets/`.

Changes:

- You can now make `.patch` files that are just merged in, early-beta-style (Kae). That's why the patch files in `assets/xSBassets` are unusually simple.
- Almost all Lua callbacks from the original xSB (by FezzedOne), `input` callbacks (by Kae), plus some extra `player` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods. The `setSpecies` and `setIdentity` callbacks will not let you switch to a nonexistent species. Documentation will be updated within the week.
- Various crash fixes (Kae and FezzedOne).
- Character swapping (rewrite by Kae from StarExtensions): `/swap <name>` (case-insensitive substring matching) and `/swapuuid <uuid>` (requires a UUID; use the one in the player file name).
- Custom user input support with a keybindings menu (rewrite by Kae from StarExtensions).
- Positional voice chat that works on completely vanilla servers; is compatible with StarExtensions. This uses Opus for crisp, HD audios. Rewrite by Kae from StarExtensions.
  - Both menus are made available in the options menu rather than as a chat command.
- Multiple font support (switch fonts inline with `^font=name;`, .ttf assets are auto-detected). By Kae.
- World lightmap generation has been moved off the main thread (Kae).
- Experimental changes to the storage of directives in memory to greatly reduce their impact on frametimes (Kae).
  - Works well when extremely long directives are used for "vanilla multiplayer-compatible" creations, like [generated](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) [clothing](https://github.com/FezzedOne/FezzedOne-Drawable-Generator).
- Client-side tile placement prediction (rewrite by Kae from StarExtensions).
  - You can also resize the placement area of tiles on the fly.
- Support for placing foreground tiles with a custom collision type (rewrite by Kae from StarExtensions; requires an OpenSB or xSB-2 server) and, via `world.placeMaterial()`, placing tiles not connected to existing ones (FezzedOne; requires an xSB-2 server). Tile placement with this feature is not network-compatible with servers that support the similar feature present in StarExtensions, although already-placed tiles work just fine. A fork of WEdit with support for these features will be on GitHub within the week.
  - Additionally, objects can be placed under non-solid foreground tiles (Kae).
- Some minor polish to UI (FezzedOne and Kae).

## Discord
 
For support, suggestions or just plain old chit-chat, check out the [xSB-2 Discord server](https://discord.gg/GJ5RTkyFCX).
