# xStarbound

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound); all credit for the original code goes to the respective authors. Contributions are welcomed. You **must** own a copy of Starbound to use xStarbound. Base game assets are not provided for obvious reasons.

**Download the latest release for Linux and Windows [here](https://github.com/xStarbound/xStarbound/releases/latest). Older releases [are also available](https://github.com/xStarbound/xStarbound/releases).**

> 🪧 **FYI:** If you're connecting to an OpenStarbound server or hosting for OpenStarbound clients, scroll down to _Network compatibility_!

## Changes

- Several new commands (by @fezzedone)! Type `/xclient` for info on the new client-side commands, or `/help` (on xServer, an xClient host or in single-player on xClient) to see the new server-side ones.
- As of v4.0+, xServer (the xStarbound server) supports scriptable functionality previously confined to wrappers like [StarryPy3k](https://github.com/StarryPy/StarryPy3k) (by @fezzedone), including:

  - Permission-based build protection for worlds and systems, complete with optional container modification/viewing protection and automatic shipworld protection and claims. Includes protection against spawning or removing entities not controlled by the player's own client (except item drops, which cannot run scripts).
  - Dedicated guest accounts and the ability to detect and handle anonymous connections in server-side scripts.
  - Scriptable connection handling, allowing mods to run, e.g., IP address checks on connecting players.
  - Account- and UUID-based world claims.
  - Chat message creation, filtering and redirection. This can be used for permission-based nickname colouring, server and world MOTDs, profanity filtering, etc.
  - Permission control, including world claim support, for entity messages handled by the server.

  See [`$docs/lua/lua.md`](doc/lua/lua.md), [`$docs/lua/world.md`](doc/lua/world.md) and [`$docs/lua/universeserver.md`](doc/lua/universeserver.md) for more.

  As of v4.1.2.3, an xServer::Helper utility mod is available. This mod adds a `/claim` command for setting world protection and claims, as well as an optional user account registration command and several admin commands for viewing and changing the server configuration without having to restart xServer. Download xServer::Helper on [the releases page](https://github.com/xStarbound/xStarbound/releases/latest).

- Persistent, scriptable server-side data storage that can be accessed and modified from any server-side universe, world or entity script (@FezzedOne). See [`$docs/lua/universeserver.md`](doc/lua/universeserver.md) for more.
- Fully scriptable world metadata modification, including support for changing any existing world's tile size (@fezzedone). Modders can now modify _all_ attributes of any existing world without resorting to external Python tools. Additionally, xClient (the xStarbound client) users can safely explore very small worlds without crashing (aside from some harmless visual glitches).
- Scriptable weather control for worlds (@Mofurka and @fezzedone) — scripts can now change a world's weather on demand.
- Nicer (and optimised) non-pixelated humanoid tech and status effect scaling for players and NPCs (reimplementation by @fezzedone).
- Now runs [Pluto](https://pluto-lang.org/), an optimised fork of Lua 5.4! As of xStarbound v4.0, Pluto's jump table optimisation is enabled, speeding up Lua scripts.
- Full Lua sandboxing! By @fezzedone.
  - To replace the old, potentially crash-prone sandbox-breaking code used by certain mods, xStarbound supports pane message handlers and Lua callbacks for safely saving and reading variables in global variable tables with the same expected cross-context scopes.
  - **Note:** This causes some mod compatibility issues; see below for affected mods.
- Full, up-to-date Lua API documentation. (Aside from a lot of engine calls into Lua scripts.)
- Support for underlaying and overlaying cosmetic items, by @fezzedone. Now includes OpenStarbound v0.1.11+ cosmetic overlay compatibility (by @novaenia and @fezzedone)! Two types of layering are supported:

  - _Underlays:_ You can right-click an item in any armour or inventory slot to _underlay_ it below any item in the associated cosmetic slot. Underlaid items have a red border by default. For instance, you can layer a wig under a hat by putting the hat in the cosmetic slot, the wig in the armour slot and right-clicking the wig. Underlaid armour grants its usual effects; note that particle effects associated with an underlaid item as `"effectSources"` won't spawn, even if the underlaid item is visible. Underlaid items are indicated with a red border.
  - _Overlays:_ You can right-click while holding **<kbd>Shift</kbd>** with an armour or clothing item in your swap slot to stack and overlay it onto another armour or clothing item of the same slot type in your armour slots or inventory — you can stack as many items as you want! To remove or «pop» the topmost item from an overlay stack, **<kbd>Shift</kbd> + Right Click** with an _empty_ swap slot — you'll see the «popped» item in your swap slot. Items are layered in the order they are stacked, with the lowest-layered item (aside from underlays) being the base item shown in the tooltip. Stacked cosmetic items have a red stack count indicator, and their tooltip preview shows what the entire stack looks like on your player. Overlaid cosmetic items do _not_ give players any additional game effects or spawn particles — they are purely cosmetic.

  Underlaid items can have overlays. This client feature is compatible with vanilla multiplayer, but other players must have xClient or OpenStarbound v0.1.11+ to see your underlays and overlays; OpenStarbound users can only see up to 12 xStarbound overlays on items in armour slots not «covered» by cosmetic slot items (regardless of underlay status), or in cosmetic slots. xStarbound users can see all OpenStarbound cosmetic overlays.

  - _Network compatibility note:_ This cross-client cosmetics compatibility does not work in the OpenStarbound-to-xStarbound direction (but does the other way around!) for OpenStarbound v0.1.11+ clients connected to OpenStarbound v0.1.11+ servers, unless the OpenStarbound clients are connected in legacy mode.

- Support for OpenStarbound v0.1.10/v0.1.11 cosmetic features (left-facing «flipped» clothing directives for custom items, humanoid config overrides, custom armour hiding, overlay-only items, middle-clicking to hide armour) by @novaenia. Tweaked for xStarbound by @fezzedone, including support for `"flipDirectives"` in `/render` and overridden movement parameters in humanoid config overrides; movement parameter override support was also added in OpenStarbound v0.1.11.1 by @novaenia.
- New optional `"xSBdirectives"` and `"xSBflipDirectives"` instance parameters for cosmetic items, by @fezzedone. These override the standard `"directives"` and `"flipDirectives"` parameters when present and support animator-like substitution tags. And yes, xClient networks these parameters and any substitutions in a fully multiplayer-compatible manner that allows stock and OpenStarbound client users to see your worn items with `"xSBdirectives"` and `"xSBflipDirectives"` exactly as they appear to xClient users, so there's no need to worry about the clients other people are using! Stock client users can even see the left-facing sprites on your flippable clothing/cosmetic items if you use `"xSBflipDirectives"` instead of `"flipDirectives"` (and you're facing left, of course). See [`$docs/cosmetics.md`](doc/cosmetics.md) for more info.
- Support for humanoid `"identity"` overrides, by @fezzedone. These allow cosmetic items to change your humanoid appearance/sprites when worn. The overrides also support substitution tags. If you use the new `"broadcast"` identity parameter, the overrides are networked in a way that allows stock and OpenStarbound client users to see them, including tag substitutions. See [`$docs/cosmetics.md`](doc/cosmetics.md) for more info.
- Control multiple characters on a single client! Is fully multiplayer-compatible. By @fezzedone. Replaces OpenStarbound's character swapping feature.
  - `/add` and `/adduuid`: Loads and adds a player character from your saves.
  - `/swap` and `/swapuuid`: Swaps to a different character. If the character isn't loaded, replaces your current character.
  - `/remove` and `/removeuuid`: Removes a character you're not currently controlling.
  - There are some game balance restrictions — dead characters won't respawn until you beam to your ship. The restrictions can be disabled via the Lua API on a per-character basis.
- A search box for long character lists. By @KrashV. Also an extra **Create Character** button while in the main menu, by @WasabiRaptor.
- An in-game character editor — use `/editor`. Works properly with modpacks too. By @fezzedone.
- xStarbound now has OpenStarbound's world file flattening and bloat fixes! By @novaenia.
- xStarbound automatically repacks shipworld and celestial world files when loading them, saving you quite a bit of disk space and, for xClient, reducing server lag caused by shipworlds. By @fezzedone.
  - Shipworld repacking is client-side; celestial world repacking is server-side.
  - Disable this automatic repacking by adding `"disableRepacking": true` to `xclient.config` or `xserver.config`.
- Various UI modding callbacks and tweaks by @grbr404, @WasabiRaptor and @Novaenia.
- Additional Lua callbacks to make player characters fully scriptable, just like NPCs! By @fezzedone.
- The UI scale can now be adjusted in the graphics settings dialogue, complete with configurable keybinds and support for fractional scales (@fezzedone). There are also keybinds for changing the in-game camera zoom (@novaenia). Both the UI scale and zoom level are scriptable (@fezzedone). UI scaling mods are no longer needed (and in fact no longer do anything) in xStarbound!
- Chat message history is now saved to `messages.json` in your storage directory instead of being reset on every disconnection (@fezzedone). Use the new `/clear` command on xClient to clear the chat history instead.
- Inventory and action bar expansion (or reduction) mods are now fully compatible with vanilla multiplayer with no changes needed on the mod's part. Additionally, these mods can now be safely added or removed without item loss or crashes as long as characters are loaded in xStarbound. Added by @WasabiRaptor and @fezzedone.
  - Loading a character after changes to inventory or action bar mods will drop any items that no longer fit on the ground beside the player (which will be picked up if there still is inventory space), instead of showing an error dialogue.
  - _Warning for users of vanilla clients and other client mods:_ If you load any characters that have their inventories resized but haven't yet dropped overflowed items on a non-xStarbound client — this can happen if you open xStarbound after changing mods, but don't actually load some characters before switching to another client — you may lose items, so load your characters in xStarbound and save your items first!
  - The networked inventory and action bar config can be configured separately with a patch to `$assets/player.config`; see [`$src/assets/xSBassets/player.config.patch`](https://github.com/fezzedone/xStarbound/blob/main/assets/xSBassets/player.config.patch) for the new parameters. Such a patch mod is _required_ for an xClient client to join a non-xServer server with inventory or action bar mods installed, but allows joining the server with mismatched mods (assuming mismatched assets are allowed).
- Anything that the game converts to a Perfectly Generic Item now has its parameters saved in the item and will be restored once any missing mods are reinstalled (@WasabiRaptor and @fezzedone). Requires xServer (or xClient on the host) for server-side items (such as those in containers on worlds, even shipworlds!) and xClient for single-player and client-side items (those in the player's inventory).
- Supports scriptable asset preprocessing. By @novaenia; fixed and greatly enhanced by @fezzedone.
- Modded techs and status effects no longer cause crashes to the menu when the offending mod is removed (@WasabiRaptor and @novaenia).
- Scriptable shader and lighting parameters are supported (@fezzedone).
- You can now make `.patch` files that are just merged in, early-beta-style (@novaenia). That's why the patch files in `assets/xSBassets` are unusually simple. All of OpenStarbound's JSON patch parameters are also supported.
- Almost all Lua callbacks from the original xSB (by @fezzedone), `input` callbacks (by @novaenia), plus some extra `player`, `chat`, `interface` and `clipboard` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods (@fezzedone).
- Various crash fixes (@fezzedone and @novaenia).
- `/settileprotection` supports variadic arguments and ranges like on OpenStarbound (@novaenia).
- Custom user input support with a keybindings menu (rewrite by @novaenia from StarExtensions).
- Client-side positional voice chat that works on completely vanilla servers; is compatible with StarExtensions. This uses Opus for crisp, HD audios. Rewrite by @novaenia from StarExtensions.
  - The voice chat configuration dialogue is made available in the options menu rather than as a chat command.
  - Extra voice chat options, including persistent saved mutes, are available with the `/voice` command (@fezzedone).
- Multiple font support (switch fonts inline with `^font=name;`, `.ttf` assets are auto-detected). Added by @novaenia, fixed by @fezzedone. Additionally, escape codes and custom fonts wrap and propagate across wrapped lines properly in the chat box (@fezzedone).
- Lighting is partially asynchronous (@novaenia).
- Various changes to the storage of directives and images in memory to greatly reduce their impact on FPS (@novaenia).
  - Works well when extremely long directives are used for «vanilla multiplayer-compatible» creations, like [generated](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) [clothing](https://github.com/fezzedone/fezzedone-Drawable-Generator).
- Client-side tile placement prediction (rewrite by @novaenia from StarExtensions).
  - You can also resize the placement area of tiles on the fly.
- Client- and server-side support for placing foreground tiles with a custom collision type (rewrite by @novaenia from StarExtensions; requires xServer or xClient on the host). Compatible with the overground placement feature of StarExtensions and OpenStarbound clients. [xWEdit](https://github.com/fezzedone/xWEdit), a fork of WEdit with support for these features, is available; xWEdit requires xClient for full client-side functionality, but partially works with OpenStarbound clients (not StarExtensions!).
  - Additionally, objects can be placed under non-solid foreground tiles (@novaenia).
- Support for placing tiles in mid-air, not connected to existing ones, via an extra argument to `world.placeMaterial()` (requires _both_ xClient and, in multiplayer, xServer/xClient on the host). By @fezzedone.
- Some polish to UI (@fezzedone and @novaenia).
- A multiplayer server list that lets you save your server IPs and accounts (@KrashV).
- Terraria-like placement animations for objects, tiles and liquids (@fezzedone). Can be disabled with an asset mod if you don't like them.
- Added Wayland support (@emanueljg).
- Nix build & install support (emanueljg). See [Nix](#nix).

## Mod compatibility

Read this to see if xStarbound is compatible with your mods.

### ☑️ Has xStarbound support

The following mods have special functionality that requires or is supported by xStarbound.

- [Actionbar Group Scrolling](https://steamcommunity.com/sharedfiles/filedetails/?id=3051031813) — fully supported by xStarbound.
- [Alternate UI Sounds [oSB]](https://steamcommunity.com/sharedfiles/filedetails/?id=3360332852) — should be supported by xStarbound; report any issues.
- [Animis](https://github.com/Lonaasan/Animis) — requires xStarbound v3.7.3+ or OpenStarbound v0.1.12+. An xStarbound compatibility patch may be required for future versions of this mod due to potential Lua context smuggling issues.
- [Armor Augment Slot](https://steamcommunity.com/sharedfiles/filedetails/?id=3448934708) — requires xStarbound or Openstarbound.
- [Auto DoubleTap Bind for Modded Techs](https://steamcommunity.com/sharedfiles/filedetails/?id=3502260176) — Requires xStarbound, OpenStarbound or StarExtensions.
- [Back Weapon II](https://steamcommunity.com/sharedfiles/filedetails/?id=3405399202) — fully supported by xStarbound.
- [Back Weapon II SChinese patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3581572023) — requires xStarbound or OpenStarbound for some of the translation patches to work properly.
- [Chaos](https://steamcommunity.com/sharedfiles/filedetails/?id=3590904263) — requires xStarbound or OpenStarbound to actually scramble treasure pools. Fun for randomiser playthroughs.
- [Clicky Keyboard UI Sounds](https://steamcommunity.com/sharedfiles/filedetails/?id=3476945792) — should be supported by xStarbound; report any issues.
- [Drop pixels on death](https://steamcommunity.com/sharedfiles/filedetails/?id=3350355857) ([GitHub](https://github.com/bongus-jive/drop-money-on-death)) — fully supported by xStarbound.
- [Dump IDs to log](https://steamcommunity.com/sharedfiles/filedetails/?id=3333016442&searchtext=) ([GitHub](https://github.com/bongus-jive/dump-ids)) — Fully supported by xStarbound as of v3.1.5r1.
- [Enhanced Storage Cumulative Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3432475751) — fully supported by xStarbound.
- [Enterable Fore Block](https://steamcommunity.com/sharedfiles/filedetails/?id=3025026792) — fully supported by xStarbound.
- [FezzedTech](https://steamcommunity.com/sharedfiles/filedetails/?id=2962923060) ([GitHub](https://github.com/fezzedone/FezzedTech)) — requires xStarbound for full functionality, but also supports OpenStarbound and StarExtensions (with reduced functionality) and is compatible with stock Starbound.
- [LR's Storage Dimension](https://steamcommunity.com/sharedfiles/filedetails/?id=3432253227) — automatic recipe detection is fully supported by xStarbound.
- [Matter Manipulator Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=3266061335) ([GitHub](https://github.com/bongus-jive/mm-keybinds/tree/main)) — fully supported by xStarbound as of v3.1.6.
- [Minecraft UI Sounds](https://steamcommunity.com/sharedfiles/filedetails/?id=3412449426) — should be supported by xStarbound; report any issues.
- [More Action Bar Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2962464896) — fully supported by xStarbound.
- [NamjeShipwright](https://github.com/namje0/namje_shipwright) — should be fully supported on xStarbound v4.1.1+. The mod is still in alpha though!
- [Neki](https://steamcommunity.com/sharedfiles/filedetails/?id=2875605913) ([GitHub](https://github.com/hyperjuni/Neki)) and [Nekibound](https://steamcommunity.com/sharedfiles/filedetails/?id=2875605913) — head rotation no longer clips Neki ears in xStarbound as of v3.4.4.
- [NEONPUNK Title Screen [oSB]](https://steamcommunity.com/sharedfiles/filedetails/?id=3359876550) — should be supported by xStarbound; report any visual issues.
- [No Food Rotting](https://steamcommunity.com/sharedfiles/filedetails/?id=3484110634) — supported by xStarbound.
- [OCD Tooltip Fix](https://steamcommunity.com/sharedfiles/filedetails/?id=3355387636) — technically supported by xStarbound, but unnecessary.
- [One-For-All Compact and Perennial Crops Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3427751671), [One-For-All Perennial Crops Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3435109352) and [One-For-All Compact Crops Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3457819726) — fully supported by xStarbound.
- [OpenStarbound WEdit](https://github.com/Mofurka/OpenStarbound-WEdit) — this WEdit fork requires OpenStarbound or xStarbound; it doesn't support xStarbound's mid-air tile placement.
- [Pan Dimensional Vending](https://steamcommunity.com/sharedfiles/filedetails/?id=3464213838&searchtext=) — fully supported by xStarbound.
- [Phantasy Starbound Title](https://steamcommunity.com/sharedfiles/filedetails/?id=3475986947) — should be supported by xStarbound; report any visual issues.
- [Planet Search](https://steamcommunity.com/sharedfiles/filedetails/?id=3269792617) — fully supported by xStarbound.
- [Quick Commands!](https://steamcommunity.com/sharedfiles/filedetails/?id=3145473452) — all OpenStarbound-compatible commands are supported by xStarbound as of v3.1.6.
- [Quick Stack Gun [OpenStarbound Fix]](https://steamcommunity.com/sharedfiles/filedetails/?id=3501752811) — Despite the name, requires xStarbound, OpenStarbound or StarExtensions.
- [Recipe Browser](https://steamcommunity.com/sharedfiles/filedetails/?id=2018183533) — Recipe Browser's universal mod support requires xStarbound or OpenStarbound.
- [RPG Growth Keybind Fix](https://steamcommunity.com/sharedfiles/filedetails/?id=3368499316) — this mod fixes a compatibility issue between RPG Growth and xStarbound/OpenStarbound/StarExtensions.
- [Ruler](https://steamcommunity.com/sharedfiles/filedetails/?id=2451043851) — fully supported by xStarbound, including keybinds, as of v3.1.6. (Bravo for getting rid of the sandbox-breaking code, Patman!)
- [Save Inventory Position](https://steamcommunity.com/sharedfiles/filedetails/?id=3331093074) ([GitHub](https://github.com/bongus-jive/save-inventory-position)) — fully supported by xStarbound. Use `/resetinventoryposition` if your inventory ends up off-screen after installation.
- [Scanner Shows Printability](https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034) — fully supported by xStarbound as of xSB v2.3.7.
- [Searchable Colony Tags](https://steamcommunity.com/sharedfiles/filedetails/?id=3496192756) — requires xStarbound or OpenStarbound.
- [Shut up about raceeffects](https://steamcommunity.com/sharedfiles/filedetails/?id=3549581457) — requires xStarbound or OpenStarbound. Recommended if you have [Frackin' Races](https://steamcommunity.com/sharedfiles/filedetails/?id=763259329) installed.
- [Size of Life - Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=3218820111), [Size of Life - Vanilla Species](https://steamcommunity.com/sharedfiles/filedetails/?id=3218826863) and other mods based on the framework — xStarbound supports «nice» non-pixelated scaling as of v2.4.1.1.
- [Spawnable Item Pack](https://steamcommunity.com/sharedfiles/filedetails/?id=733665104) — SIP's universal mod support requires xStarbound v2.5+ or OpenStarbound.
- [Starburst Rework T6 Armor Recipe Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3472326270) — requires xStarbound or OpenStarbound.
- [StarCustomChat](https://steamcommunity.com/sharedfiles/filedetails/?id=3208917628) ([GitHub](https://github.com/KrashV/StarCustomChat)) and [StarCustomChatRP](https://steamcommunity.com/sharedfiles/filedetails/?id=3445409664) ([GitHub](https://github.com/KrashV/StarCustomChatRP)) — requires xStarbound v3.5.1+, OpenStarbound v0.1.8+ or StarExtensions.
- [Tech Loadout Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2920684844) — fully supported by xStarbound.
- [The Hungercry Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=3594407068) — requires xStarbound, OpenStarbound or StarExtensions for a `player` callback, despite not being tagged as such.
- [Time Control Command](https://steamcommunity.com/sharedfiles/filedetails/?id=3256623666) ([GitHub](https://github.com/bongus-jive/TimeControlCommand)) — fully supported by xStarbound.
- [Unde Venis](https://steamcommunity.com/sharedfiles/filedetails/?id=3425456029) — fully supported by xStarbound as of xSB v3.4.2.
- [Universal Keybind Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3466851780) ([GitHub](https://github.com/FezzedOne/Starbound-Universal-Keybind-Compatibility)) — requires xStarbound or OpenStarbound. Install this mod if you have issues with getting mod keybinds, like RPG Growth's tech keybinds, to work on xStarbound or OpenStarbound.
- [Universal Upgradeable Weapons Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3595603580) — requires xStarbound or OpenStarbound to do anything.
- [Wardrobe Cumulative Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3433498458) — supported by xStarbound as of xSB v3.4.4.2.
- [xDPC](https://github.com/FezzedOne/xDPC) — requires xStarbound for full support, but also works on OpenStarbound and StarExtensions, albeit with a few missing features.
- [xSIP](https://github.com/fezzedone/xSIP) — xSIP's universal mod support requires xStarbound v2.5+ or OpenStarbound.
- [xWEdit](https://github.com/fezzedone/xWEdit) — this WEdit fork requires xStarbound for full functionality, but is partially supported by OpenStarbound (no mid-air tile placement) and compatible with vanilla Starbound (with no extra functionality above WEdit).
- Mods that change the size or number of bags in the inventory or hotbar — as of xSB v2.4, xStarbound gives these mods full compatibility with vanilla multiplayer and existing characters «out of the box».

### Requires xSBCompat

xStarbound now has a «universal mod compatibiliser» called xSBCompat! xSBCompat is available as a separate download on the xStarbound downloads page, and _requires_ xStarbound, so it should be installed in your `xsb-assets/` directory. The following mods require xSBCompat to resolve compatibility issues:

- [All Items are Stackable!](https://steamcommunity.com/sharedfiles/filedetails/?id=3370469697) — needs xSBCompat to remove an unnecessary OpenStarbound check.
- [Automatically Scan Objects!](https://steamcommunity.com/sharedfiles/filedetails/?id=3545869822) — ditto.
- [Betabound!](https://steamcommunity.com/workshop/filedetails/?id=2010607826) — xSBCompat fixes compatibility issues, but doesn't enable a few OpenStarbound-specific bits and bobs.
- [boner guy](https://steamcommunity.com/sharedfiles/filedetails/?id=2992238651) — no comment.
- [Cheap as Dirt](https://steamcommunity.com/sharedfiles/filedetails/?id=3302756487) — xSBCompat required to render an OpenStarbound check toothless.
- [Dynamic Proximity Chat](https://steamcommunity.com/sharedfiles/filedetails/?id=3450266347) ([GitHub](https://github.com/cptsalt/Dynamic-Proximity-Chat)) — may have minor compatibility issues even with xSBCompat. xStarbound users should use FezzedOne's fork [xDPC](https://github.com/FezzedOne/xDPC), which does not require xSBcompat, instead, unless the «mainline» Dynamic Proximity Chat is required for a server. Note that xDPC isn't fully network-compatible with Dynamic Proximity Chat!
- [Hunger Fighting Chairs](https://steamcommunity.com/sharedfiles/filedetails/?id=3546473893) — requires xSBCompat to defang an unnecessary OpenStarbound check.
- [Infinite Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=1944652893) — xSBCompat adds support for the keybind.
- [Infiniter Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=3514830972) ([GitHub](https://github.com/bongus-jive/infiniter-inventory)) — all features supported.
- [Is this Printable?](https://steamcommunity.com/sharedfiles/filedetails/?id=3507216031) — requires xSBCompat to bypass unnecessary OpenStarbound checks.
- [Limited Lives](https://steamcommunity.com/sharedfiles/filedetails/?id=3222951645) — needs xSBCompat to defang an unneeded OpenStarbound check.
- [Nodachi](https://steamcommunity.com/sharedfiles/filedetails/?id=2995409356) — the patch fixes a typo that causes an xStarbound compatibility issue.
- [NPC Mechs](https://steamcommunity.com/sharedfiles/filedetails/?id=1788644520) — ditto.
- [Save Inventory Position](https://steamcommunity.com/sharedfiles/filedetails/?id=3331093074) ([GitHub](https://github.com/bongus-jive/save-inventory-position)) — fully supported with xSBCompat. Use `/resetinventoryposition` if your inventory ends up off-screen after installation.
- [Ship Pet Swapper](https://steamcommunity.com/sharedfiles/filedetails/?id=3474107812) — needs xSBCompat to remove yet another unnecessary OpenStarbound check.
- [Unde Venis](https://steamcommunity.com/sharedfiles/filedetails/?id=3425456029) — fully supported once patched.
- [Universal Instant Crafting for All Mods](https://steamcommunity.com/sharedfiles/filedetails/?id=3251274439) — requires xSBCompat to bypass unnecessary OpenStarbound checks.
- [Unlimited Food Stacking](https://steamcommunity.com/sharedfiles/filedetails/?id=3301942276) — Ditto.
- [ZB SAIL: Standalone](https://steamcommunity.com/sharedfiles/filedetails/?id=3336389472) — Ditto.

Feel free to ask @FezzedOne about adding support for mods that are partially or totally incompatible with xStarbound to this patch, or contribute your own PR!

> **Note:** xSBCompat support for Stardust Core and Quickbar Mini is _not_ planned due to those mods' heavy reliance on engine exploits that are no longer supported by xStarbound and unfortunately require extensive, painful workarounds to patch out. Additionally, the authors of many of the mods listed above will _not_ respond (or will respond in stupid ways) to xStarbound-related support requests for stupid reasons, so don't expect «first-party» support for mods covered by this patch.

### ✅ Compatible

Any mod not listed as «partially compatible», «not compatible» or «requires xSBCompat» category should be compatible with xStarbound. Major mods that have been tested to be compatible:

- [Arcana](https://steamcommunity.com/workshop/filedetails/?id=2359135864) — use [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) to satisfy this mod's «Stardust Core Lite» dependency (see that mod below for why).
- [Avali (Triage) Race Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=729558042).
- [Elithian Races Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=850109963).
- [Frackin' Universe](https://steamcommunity.com/sharedfiles/filedetails/?id=729480149) ([GitHub](https://github.com/sayterdarkwynd/FrackinUniverse)). **Requires the Frackin' Patch on the [downloads page](https://github.com/xStarbound/xStarbound/releases/latest) to fix a compatibility issue between xStarbound's sandbox and FU's power objects.**
- [Infinite Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=1944652893) — works, but install xSBCompat to remove an unnecessary check that prevents the keybind from working on xStarbound.
- [Maple32](https://steamcommunity.com/sharedfiles/filedetails/?id=2568667104&searchtext=maple32).
- [OpenStarbound No Highlights on Scanned Objects and Players](https://steamcommunity.com/sharedfiles/filedetails/?id=3432675895) — compatible. Has no particular OpenStarbound or xStarbound support, actually.
- [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034). Note that the Classic Quickbar extension (see below) has sandbox-related compatibility issues.
- [Shellguard: Starbound Expansion Remastered](https://steamcommunity.com/sharedfiles/filedetails/?id=1563376005).
- [Updated Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=2641776549) — use this instead of the incompatible Classic Quickbar (see below).
- [WTM (WTM Teleporter Mod)](https://steamcommunity.com/sharedfiles/filedetails/?id=1268222595) — verified compatible; the developer Hiran is also behind Digital Storage (see below).

> **Note:** xStarbound does not and will not support StarExtensions' «body dynamics» and text-to-speech features, nor will it support OpenStarbound v0.1.15+'s scriptable humanoid animation system. Details:
>
> - Armour, clothing and race mods with included SE «body dynamics» support are compatible, but the «non-jiggle» sprites will be displayed.
> - Mods intended to patch in «body dynamics» support for other mods simply will not work at all.
> - Race mods that support StarExtensions' text-to-speech feature will work just fine, but the text-to-speech functionality won't work.
> - Race mods that use the scriptable humanoid animation system in OpenStarbound v0.1.15+ will not have correct humanoid rendering on xStarbound and may even throw a fatal error on startup due to missing expected parameters.

### ⚠️ Partially compatible

The following mods are only partially compatible with xStarbound:

- [1x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=1782208070) — won't do anything and is redundant.
- [3x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2681858844) — ditto.
- [4x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2870596125) — ditto.
- [OpenUI](https://steamcommunity.com/workshop/filedetails/?id=3546647977), its [Race Extender](https://steamcommunity.com/sharedfiles/filedetails/?id=3546654953) and other OpenUI extensions — these mods require OpenStarbound; they work on xStarbound, but have glaring visual issues with certain xStarbound dialogues and menus (namely, the main settings dialogue, server connection dialogue and escape/pause menu) that warrant placing these mods in the _Partially compatible_ category for now.
- [Optimizebound](https://steamcommunity.com/sharedfiles/filedetails/?id=902555153) and its [two](https://steamcommunity.com/sharedfiles/filedetails/?id=2954344118) [counterparts](https://steamcommunity.com/sharedfiles/filedetails/?id=2954354494) — their PNG compression does not play well with xClient's hardware cursor, potentially making it invisible. Use `/cursor off` if you want to use these mods.
- [Project Knightfall](https://steamcommunity.com/sharedfiles/filedetails/?id=2010883172) ([GitHub](https://github.com/Nitrosteel/Project-Knightfall)) — some added UIs have minor issues with xStarbound's Lua sandbox.
- [Русификатор Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2980671752) — technically fully compatible, but the mod it patches isn't compatible anyway.
- [Starloader](https://steamcommunity.com/sharedfiles/filedetails/?id=2936533996) ([GitHub](https://github.com/Starbound-Neon/StarLoader)) — fully compatible as long as `"safeScripts"` is disabled in your `xclient.config` (but be careful with that!).
- [Unitilities | Lua Modding Library](https://steamcommunity.com/sharedfiles/filedetails/?id=2826961297) — the Hasibound-specific functionality is not supported by xStarbound.
- Other UI scaling mods — these won't do anything and are redundant. Mods that do other stuff besides scaling the UI should work.

### ❌ Not compatible

The following mods are _NOT_ compatible with xStarbound, even with xSBCompat:

- [AR's Shader Pack v1.0](https://steamcommunity.com/sharedfiles/filedetails/?id=3487232242) — modular shader support is OpenStarbound-only.
- [Beta Hotbar!](https://steamcommunity.com/sharedfiles/filedetails/?id=3472065640) — not compatible with xStarbound's Lua sandbox. xSBCompat support may be added in the future.
- [Bottinator22's](https://steamcommunity.com/sharedfiles/filedetails/?id=3431152501) [shader](https://steamcommunity.com/sharedfiles/filedetails/?id=3431151263) [mods](https://steamcommunity.com/sharedfiles/filedetails/?id=3431151049) — modular shader support is OpenStarbound-only.
- [Bott's Shaders - Disabled by Default](https://steamcommunity.com/sharedfiles/filedetails/?id=3468244512) — the mods it depends on are OpenStarbound-only, obviously.
- [Classic Quickbar](https://steamcommunity.com/sharedfiles/filedetails/?id=2957136802) ([GitHub](https://github.com/bongus-jive/classic-quickbar)) — not compatible with xStarbound's Lua sandbox. Use [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) instead.
- [Cumulative Dynamic Lights](https://steamcommunity.com/sharedfiles/filedetails/?id=3444407977) — not compatible because it depends on OpenStarbound's lighting system changes. Won't do much on xStarbound.
- [DigitalCrafting Support Patch](https://steamcommunity.com/sharedfiles/filedetails/?id=3519204938) — This mod depends on [DigitalCrafting](https://steamcommunity.com/workshop/filedetails/?id=2919830263), which itself depends on Digital Storage (see below), and is therefore incompatible with xStarbound for that reason.
- [Digital Storage](https://steamcommunity.com/sharedfiles/filedetails/?id=946227505) — the server-side scripts are not compatible with xStarbound's Lua sandbox. Not covered by xSBscripts due to very gnarly code.
- [Futara's Dragon Engine](https://steamcommunity.com/workshop/filedetails/?id=2297133082), [Futara's Dragon Race](https://steamcommunity.com/sharedfiles/filedetails/?id=1958993491) and Futara's other mods that depend on the Dragon Engine — not compatible with xStarbound's Lua sandbox. Fixing this would require a significant rewrite of Futara's Dragon Engine's code on Futara's end, so no compatibility patch is forthcoming. Sorry!
- [Harder Core Mode](https://steamcommunity.com/sharedfiles/filedetails/?id=3527438945) — not compatible with xStarbound's Lua sandbox. Ask about xSBCompat support.
- [inventory scrambler](https://steamcommunity.com/sharedfiles/filedetails/?id=3538536096) ([GitHub](https://github.com/bongus-jive/inventory-scrambler)) — not compatible with xStarbound's Lua sandbox. Ask about xSBCompat support, but seriously, why would you want to scramble your inventory?
- [Light Limiter](https://steamcommunity.com/sharedfiles/filedetails/?id=3470727854) — neither compatible with nor necessary on xStarbound.
- [OpenStarbound Revert](https://steamcommunity.com/sharedfiles/filedetails/?id=3509533339) — not compatible with xStarbound for obvious reasons. No compatibility patch is planned. Why do you need this on xStarbound anyway?
- [Psychedelic Shader](https://steamcommunity.com/sharedfiles/filedetails/?id=3573193020) — modular shader support is OpenStarbound-only.
- [Raptor's Metroid Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=3541573028) — certain scripting functionality required by this mod is not currently present in xStarbound; this may change in the future.
- [Remote Module](https://steamcommunity.com/sharedfiles/filedetails/?id=2943917766) — won't work and is likely to log script errors.
- [Stardust Core](https://steamcommunity.com/sharedfiles/filedetails/?id=764887546) and [Stardust Core Lite](https://steamcommunity.com/sharedfiles/filedetails/?id=2512589532) ([GitHub](https://github.com/zetaPRIME/sb.StardustSuite)) — these mods' scripts are not compatible with xStarbound's Lua sandbox, and xSBCompat support is _not_ planned. Use [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) instead.
- [Stardust Suite](https://github.com/zetaPRIME/sb.StardustSuite) ([GitHub](https://github.com/zetaPRIME/sb.StardustSuite)) — Ditto.
- [StarExtensions](https://github.com/StarExtensions/StarExtensions) — won't load on xClient and may cause crashes! However, xServer fully supports the server-side part of SE's «overground» tile placement feature.
- [Text to Speech Droids](https://steamcommunity.com/sharedfiles/filedetails/?id=2933125939) — won't do anything.
- [Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2916058850) — will log script errors (xStarbound has differently named callbacks) and is redundant anyway because xStarbound already fully supports this feature.
- Mods that patch in StarExtensions «body dynamics» support for other mods. These won't do anything.

## Network compatibility

Due to recent changes in OpenStarbound's network protocol, xStarbound's network protocol is no longer fully network-compatible with OpenStarbound. As of v3.3.3r1, there are compatibility workarounds for this:

- If you're getting a «No server response received» error on xClient when connecting to an OpenStarbound server, try putting an `@` before the address in the **Multiplayer** box; e.g., `@sb.some-server.net` instead of `sb.some-server.net`. This tells xClient to pretend to be a vanilla client for networking purposes, and should let you connect.
- If you're using an OpenStarbound v0.1.9+ client to connect to an xStarbound server, tick the «FORCE LEGACY» box on the connection screen before connecting.
- If players with OpenStarbound clients are having trouble connecting to your xServer server or xClient Steam host, try adding `"forceLegacyConnection": true` to your server's `xserver.config` or host client's `xclient.config`. This tells xStarbound to pretend to be a vanilla server or host for networking purposes, and should let those players connect. (Enabling this setting on xClient also tells the client to always pretend to be a vanilla client for networking purposes.)

## Building

This repository is already set up for easy building (mostly @kblaschke's work, with an updated Windows build script and some additional fixes by @fezzedone). Follow the appropriate instructions for your OS if listed; if your OS _isn't_ listed, adjustments generally shouldn't be too complex.

### Linux

On Linux, the xStarbound binaries are by default built against the system libraries. To build xStarbound on any reasonably up-to-date Linux install (assuming `bash` or `zsh`):

Wayland should work OOTB if you're using the `xclient.sh`. If you're running the binary yourself, you can enable this support yourself with `SDL_VIDEODRIVER="wayland"`.

1. If you're on SteamOS, run `sudo steamos-readonly disable`.
2. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
3. Install CMake, Git and the required build libraries for xStarbound:
   - _Arch-based distros (CachyOS, Endeavour, etc.):_ `sudo pacman -S clang cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland qt6-svg qt6-base sdl2 libpng freetype2 libvorbis opus glew` (you may need to `-Syu` first)
   - _RPM/`dnf`-based distros:_ `sudo dnf install gcc-c++ clang cmake git ninja-build mesa-libGLU libXrender libXi libxkbcommon egl-wayland qt6-qtbase qt6-qtsvg SDL2-devel libpng-devel freetype-devel libvorbis-devel opus-devel glew-devel` (yes, Fedora doesn't come with a C++ compiler preinstalled!)
   - _Debian/`apt`-based distros:_ `sudo apt install clang cmake git ninja-build build-essential libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev qt6-base qt6-svg libsdl2-dev libpng-dev libfreetype6-dev libvorbis-dev libopus-dev libglew-dev`
   - _Gentoo:_ `sudo emerge -a sys-devel/clang dev-vcs/git dev-build/cmake dev-build/ninja media-libs/mesa virtual/glu x11-misc/xcb x11-libs/libGLw x11-libs/libXrender x11-libs/libXi x11-libs/libxkbcommon gui-libs/egl-wayland media-libs/libsdl2 dev-qt/qtbase dev-qt/qtsvg media-libs/freetype media-libs/libvorbis media-libs/libpng media-libs/opus media-libs/glew`
   - _SteamOS:_ `sudo steamos-readonly disable; sudo pacman -Syu clang cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland qt6-svg qt6-base sdl2 libpng freetype2 libvorbis opus; sudo steamos-readonly enable`
4. `git clone --recursive https://github.com/xStarbound/xStarbound.git` (`--recursive` is needed to clone the Opus sources in `$src/extern/opus`, which may be necessary on some configurations)
5. `cd xStarbound/`
6. `CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DCMAKE_BUILD_TYPE=Release -DSTAR_ENABLE_STEAM_INTEGRATION=ON -DSTAR_USE_RPMALLOC=ON -DPACKAGE_XSB_ASSETS=ON -S . -B build/ -G Ninja` (Clang is recommended now, but you can use `gcc` and `g++` instead for GCC)
7. `cmake --build build/`
8. `cmake --install build/ --prefix ${sbInstall}/` (replace `${sbInstall}` with the path to your Starbound install)
9. `cp scripts/linux/{xclient,xserver,mod_uploader}.sh ${sbInstall}/linux/`
10. Run `./xclient.sh` to play or `./xserver.sh` to host the server! Optionally configure Steam or your other launcher to launch `${sbInstall}/linux/xclient.sh`.

> **Important:** If you're getting library linking errors while attempting to build or run xStarbound (this is likely on Debian-based distros, Slackware and CentOS due to their older libraries) or your distro is old enough to still use `yum` or `apt-get`, you'll need to either build a statically linked version of xStarbound (see below), figure out how to build xStarbound against the Steam runtime (hint: update the runtime's CMake somehow!) or find a way to update your system libraries.

#### Statically linked builds

To build a statically linked version of xStarbound (assuming `bash` or `zsh`):

> **Important:** It is _highly_ recommended you use an older LTS distribution such as Debian 11 or 12, or Ubuntu 20.x or 22.x, in order to guarantee portability. You can use a Docker, Toolbox or Podman container for this. If the chosen distro does not have CMake 4.0+ available in its repositories, download the latest `tar.gz` archive from [CMake's website](https://cmake.org/download/), extract it somewhere, and add its `bin/` directory to your `$PATH` _instead_ of installing it via the package manager.

1. If you're on SteamOS, run `sudo steamos-readonly disable`.
2. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
3. Install CMake, Git and the required build libraries for xStarbound:
   - _Arch-based distros (CachyOS, Endeavour, etc.):_ `sudo pacman -S clang cmake git ninja patchelf mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland` (you may need to `-Syu` first)
   - _RPM/`dnf`-based distros:_ `sudo dnf install gcc-c++ clang cmake git ninja-build patchelf mesa-libGLU libXrender libXi libxkbcommon egl-wayland xcb* libX11-xcb` (yes, Fedora doesn't come with a compiler preinstalled!)
   - _Debian/`apt`-based distros:_ `sudo apt install clang cmake git ninja-build patchelf build-essential libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev libxcb-*`
   - _Gentoo:_ `sudo emerge -a sys-devel/clang dev-vcs/git dev-build/cmake dev-build/ninja dev-util/patchelf media-libs/mesa virtual/glu x11-misc/xcb x11-libs/libGLw x11-libs/libXrender x11-libs/libXi x11-libs/libxkbcommon gui-libs/egl-wayland`
   - _SteamOS:_ `sudo steamos-readonly disable; sudo pacman -Syu clang cmake git ninja patchelf mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland; sudo steamos-readonly enable`
4. `mkdir -p ~/.local/opt; git clone https://github.com/microsoft/vcpkg.git ~/.local/opt/vcpkg`
5. `cd ~/.local/opt/vcpkg; ./bootstrap-vcpkg.sh -disableMetrics` (yes, VCPKG sends telemetry by default)
6. `cd $devDirectory` (where `$devDirectory` is the folder you want to put the xStarbound source in)
7. `git clone --recursive https://github.com/xStarbound/xStarbound.git` (`--recursive` is needed to clone the Opus sources in `$src/extern/opus`, which may be necessary on some configurations)
8. `cd xStarbound/; export VCPKG_ROOT="${HOME}/.local/opt/vcpkg"; export PATH="${VCPKG_ROOT}:${PATH}"`
9. `CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake --build cmake-build-linux-x86_64/ --preset "linux-vcpkg-x86_64-release" -G Ninja` (Clang is recommended on up-to-date distros; GCC — `gcc` and `g++` — on less up-to-date ones)
10. `cmake --install cmake-build-linux-x86_64/ --prefix ${sbInstall}/` (replace `${sbInstall}` with the path to your Starbound install)
11. `cp scripts/linux/{xclient-static,xserver,mod_uploader}.sh ${sbInstall}/linux/`
12. `mv ${sbInstall}/linux/xclient-static.sh ${sbInstall}/linux/xclient.sh`
13. Optionally configure Steam or your other launcher to launch `${sbInstall}/xsb-linux/xclient.sh`.

### Nix / NixOS

See [`$xsbSrc/nix/README.md`](nix/README.md) as well as the [`nix/` directory](nix/).

### Windows 10 or 11

To build and install xStarbound on Windows 10 or 11:

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/whatsnew/) and [CMake](https://cmake.org/download/). For Visual Studio, make sure to install C++ support (and optionally the game dev stuff) when the VS installer asks you what to install. For CMake, make sure you download and use the `.msi` installer for 64-bit Windows.
2. Install [Git](https://git-scm.com/download/win); the build script _requires_ Git. On the «Adjusting your PATH environment» page of the installation wizard, make sure to select the _second_ option[^gitOptions], which the wizard recommends anyway.
3. Open up Git Bash and run `git clone --recursive https://github.com/xStarbound/xStarbound.git`.
4. Go into `scripts\windows\` and double-click `build.bat`. CMake and VCPKG will take care of the entire build process.
5. If the build succeeds, you will be prompted to select your Starbound install directory.
6. Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

[^gitOptions]: Linux/WSL nerds _can_ use the third option; xStarbound's build script doesn't depend on the overridden CLI tools.

> **Building on older Windows versions:** Building on earlier versions of Windows is _not_ recommended, although it _should_ still be possible to build xStarbound on Windows 7, 8 or 8.1 if you can get VS 2022 installed.

### Other OSes

The basic process for building on other OSes:

1. Install CMake, a C++ compiler toolchain and (optionally) Git.
   - On _macOS 10.11+_, you should use [Homebrew](https://brew.sh/) to install CMake, Git and the needed build dependencies on your system. Manual installation of dependencies via wizards or drag-and-drop is likely to cause build issues or be a pain to work with.
   - If you're targeting _older versions of macOS / OS X_, look into [Tigerbrew](https://github.com/mistydemeo/tigerbrew). Expect to do more tinkering and/or manual installation of build dependencies.
   - Most _BSDs_ and other reasonably up-to-date _\*nix OSes_ (like Haiku) should have the necessary packages in their ports system or equivalent. Make sure to disable Steam and Discord integrations.
   - For _Windows XP_ or _Vista_ targets, your best bet is to use Visual Studio 2017 or 2019 on a newer version of Windows or on WINE with the `v141_xp` toolchain (it doesn't work on VS 2022!). Make sure to disable Steam and Discord integrations.
   - Targetting _older Windows versions_, _pre-OSX Mac OS_, _Android_, the _Switch_ or any other niche device or OS is left as an exercise for the reader.
2. If you can, install any dependencies needed to build SDL2 and GLEW on your OS. If you can't, hopefully VCPKG runs on your build OS and/or has packages for your target OS. If not, get ready for a lot of tinkering.
3. If you're not using Git or don't have it installed, download the latest xStarbound source ZIP, extract it somewhere and go to step 5. Otherwise go to the next step.
4. If you're using Git, run `git clone https://github.com/xStarbound/xStarbound.git` in a terminal or command prompt, or use a graphical Git utility to download the repo.
5. In the xStarbound directory, run `cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/`. On some OSes, you may need to add the full path to your CMake executable to the beginning of the command. If necessary, add `-DCMAKE_C_COMPILER=<path to C++ compiler> -DCMAKE_CXX_COMPILER=<path to C++ compiler>`. Note that a CMake build preset already exists for modern macOS; consider using that if you have build issues.

### Packaging and installing xSB assets

The build system can automatically take care of packaging the additional xSB assets and install the resulting .pak file automatically. To enable asset packing, pass `-DPACKAGE_XSB_ASSETS=ON` to the CMake command, e.g. after the `--preset` argument. This will create a directory `xsb-assets` in the project's main dir, and package the resources in `assets/xSBassets` into a `packed.pak` file.

It is important to note that automatic packaging will _only_ work if the built binaries (specifically `asset_packer[.exe]`) run on the host system. If you are cross-compiling, e.g. for a different CPU architecture (building for ARM64 on a x64 OS) or a different OS (building with MinGW on Linux, or with WSL on Windows), this feature cannot be used. In this case, you have to manually package the assets with an asset*packer that runs locally, and copy the resulting file to `xsb-assets/packed.pak` \_before installing the project*. Due to how CPack works, this is only feasible when packaging manually from the install dir.

### Tracy profiler support

xStarbound now supports [the Tracy profiler](https://github.com/wolfpld/tracy)! To build xStarbound with Tracy, tack `-DXSB_ENABLE_TRACY` onto the end of the _first_ CMake configuration command from the appropriate build instructions for your OS/platform.

To build the Tracy profiler on Linux, `cd $xsb/tracy`, then `mkdir -p build; cmake -B build/ -DCMAKE_BUILD_TYPE=Release; cmake --build build/ --config Release --parallel`. The profiler binary will be a file called `tracy-profiler` in `$xsb/tracy/build/`; feel free to move this to `~/.local/bin/` or wherever else you find convenient.

Windows users can download a prebuilt Tracy profiler [here](https://github.com/wolfpld/tracy/releases/latest) or build it themselves using the build instructions in section 2.3 of [Tracy's manual](tracy.pdf).

#### Tracy Pluto/Lua callbacks

`tracy` callbacks are now available in all script contexts on Tracy-instrumented builds of xStarbound, allowing modders to instrument and profile their scripts. See section 3.12 of [Tracy's manual](tracy.pdf) for more information. (Don't forget to use an `if tracy` clause to check if you're on a Tracy-instrumented build first!)

## Discord

For support, suggestions or just plain old chit-chat, check out the [xStarbound Discord server](https://discord.gg/GJ5RTkyFCX).

## Font attributions

Attributions for the fonts that come with xStarbound:

- **7 Segmental Digital Display:** By Ash Pikachu Font. Link: https://www.dafont.com/7-segmental-digital-display.font
- **Beech:** By Yuji Oshimoto. Link: http://04.jp.org/ [under **font ps&tt**] (Licence: http://www.dsg4.com/04/extra/font/about.html)
- **Avali Scratch:** By AikaDee. Link: https://fontstruct.com/fontstructions/show/1108804/avali_scratch
- **Free Pixel:** By levelb. Link: https://www.dafont.com/free-pixel.font
- **Hobo (pixelised Koala build version):** By Chucklefish, of course.
- **Iosevka:** By @be5invis. Link: https://typeof.net/Iosevka/ (GitHub link: https://github.com/be5invis/Iosevka)
- **LED Real:** By Matthew Welch. Link: https://www.dafont.com/led-real.font
- **Libre Barcode 128:** By Lasse Fister. Link: https://fonts.google.com/specimen/Libre+Barcode+128
- **Libre Barcode 128 Text:** By Lasse Fister. Link: https://fonts.google.com/specimen/Libre+Barcode+128+Text
- **Moder Dos:** By Grandoplex Productions. Link: https://fontmeme.com/fonts/moder-dos-437-font/
- **Newspaper Cutout White on Black:** By onlygfx.com. Link: https://www.dafont.com/newspaper-cutout-white-on-black.font
- **Pixel Arial 11:** By «Max». Link: https://www.dafont.com/pixel-arial-11.font
- **Pixel Operator:** By Jayvee Enaguas. Link: https://www.dafont.com/pixel-operator.font
- **Pixolde:** By jeti. Link: https://www.dafont.com/pixolde.font
- **Roses are FF0000:** By AJ Paglia. Link: https://www.dafont.com/roses-are-ff0000.font
- **Runescape UF:** By Nathan P. Link: https://www.dafont.com/runescape-uf.font
- **Smooth Hobo:** By The_Pro. Link to Steam community page: https://steamcommunity.com/profiles/76561198054550016
- **Space Mono:** By Colophon Foundry. Link: https://fonts.google.com/specimen/Space+Mono
- **UnifontEX:** By @stgiga (fork of GNU Unifont). Link: https://stgiga.github.io/UnifontEX/ (GitHub: https://github.com/stgiga/UnifontEX)
- **VCR OSD Mono:** By Riciery Leal. Link: https://www.dafont.com/vcr-osd-mono.font
