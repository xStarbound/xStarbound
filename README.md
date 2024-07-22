# xStarbound

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xStarbound. Base game assets are not provided for obvious reasons.

Compiled builds for Linux and Windows should be available in the usual place on this repository.

## Changes

- Several new commands (by FezzedOne)! Type `/xclient` for info on the new client-side commands, or `/help` (on xServer, an xClient host or in single-player on xClient) to see the new server-side ones.
- Nicer (and optimised) non-pixelated humanoid tech and status effect scaling for players and NPCs (reimplementation by FezzedOne).
- Now runs [Pluto](https://pluto-lang.org/), a fork of Lua 5.4!
- Full Lua sandboxing when `"safeScripts"` is enabled! By FezzedOne.
  - To replace the old, potentially crash-prone sandbox-breaking code used by certain mods, new Lua callbacks for safely saving and reading variables in global variable tables with the same expected cross-context scopes.
  - **Note:** This causes some mod compatibility issues; see below for affected mods. A patch to fix them will be released soon.
- Full, up-to-date Lua API documentation. (Aside from a lot of engine calls into Lua scripts.)
- Control multiple characters on a single client! Is fully multiplayer-compatible. By FezzedOne. Replaces OpenStarbound's character swapping feature.
  - `/add` and `/adduuid`: Loads and adds a player character from your saves.
  - `/swap` and `/swapuuid`: Swaps to a different character. If the character isn't loaded, replaces your current character.
  - `/remove` and `/removeuuid`: Removes a character you're not currently controlling.
  - There are some game balance restrictions — dead characters won't respawn until you beam to your ship. The restrictions can be disabled via the Lua API on a per-character basis.
- xStarbound automatically repacks shipworld and celestial world files when loading them, saving you quite a bit of disk space and, for xClient, reducing server lag caused by shipworlds. By FezzedOne.
  - Shipworld repacking is client-side; celestial world repacking is server-side.
  - Disable this automatic repacking by adding `"disableRepacking": true` to `xclient.config` or `xserver.config`.
- Additional Lua callbacks to make player characters fully scriptable, just like NPCs!
- The UI scale can now be adjusted in the graphics settings dialogue, complete with configurable keybinds and support for fractional scales (FezzedOne). There are also keybinds for changing the in-game camera zoom (Kae). Both the UI scale and zoom level are scriptable (FezzedOne). UI scaling mods are no longer needed (and in fact no longer do anything) in xStarbound!
- Chat message history is now saved to `messages.json` in your storage directory instead of being reset on every disconnection. Use the new `/clear` command on xClient to clear the chat history instead.
- Inventory and action bar expansion (or reduction) mods are now fully compatible with vanilla multiplayer with no changes needed on the mod's part. Additionally, these mods can now be safely added or removed without item loss or crashes as long as characters are loaded in xStarbound. Added by WasabiRaptor and FezzedOne.
  - Loading a character after changes to inventory or action bar mods will drop any items that no longer fit on the ground beside the player (which will be picked up if there still is inventory space), instead of showing an error dialogue.
  - *Warning for users of vanilla clients and other client mods:* If you load any characters that have their inventories resized but haven't yet dropped overflowed items on a non-xStarbound client — this can happen if you open xStarbound after changing mods, but don't actually load some characters before switching to another client — you may lose items, so load your characters in xStarbound and save your items first!
  - The networked inventory and action bar config can be configured separately with a patch to `$assets/player.config`; see [`$src/assets/xSBassets/player.config.patch`](https://github.com/FezzedOne/xStarbound/blob/main/assets/xSBassets/player.config.patch) for the new parameters. Such a patch mod is *required* for an xClient client to join a non-xServer server with inventory or action bar mods installed, but allows joining the server with mismatched mods (assuming mismatched assets are allowed).
- Anything that the game converts to a Perfectly Generic Item now has its parameters saved in the item and will be restored once any missing mods are reinstalled (WasabiRaptor and FezzedOne). Requires xServer (or xClient on the host) for server-side items (such as those in containers on worlds, even shipworlds!) and xClient for single-player and client-side items (those in the player's inventory).
- Supports scriptable asset preprocessing. By Kae; fixed and greatly enhanced by FezzedOne.
- Modded techs and status effects no longer cause crashes to the menu when the offending mod is removed (Kae and WasabiRaptor).
- Scriptable shader and lighting parameters are supported (FezzedOne).
- You can now make `.patch` files that are just merged in, early-beta-style (Kae). That's why the patch files in `assets/xSBassets` are unusually simple. In addition, a new `"find"` parameter is supported for `"remove"` and `"replace"` operations where the `"path"` is to a JSON array (FezzedOne) — if `"find"` is present, only the first array value exactly matching the value in `"find"` is removed or replaced by `"value"`, not the entire array.
- Almost all Lua callbacks from the original xSB (by FezzedOne), `input` callbacks (by Kae), plus some extra `player`, `chat`, `interface` and `clipboard` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods (FezzedOne). The `setSpecies` and `setIdentity` callbacks will not let you switch to a nonexistent species (FezzedOne). Documentation has yet to be updated.
- Various crash fixes (FezzedOne and Kae).
- Custom user input support with a keybindings menu (rewrite by Kae from StarExtensions).
- Client-side positional voice chat that works on completely vanilla servers; is compatible with StarExtensions. This uses Opus for crisp, HD audios. Rewrite by Kae from StarExtensions.
  - The voice chat configuration dialogue is made available in the options menu rather than as a chat command.
  - Extra voice chat options, including persistent saved mutes, are available with the `/voice` command (FezzedOne).
- Multiple font support (switch fonts inline with `^font=name;`, `.ttf` assets are auto-detected). Added by Kae, fixed by FezzedOne. Additionally, escape codes and custom fonts wrap and propagate across wrapped lines properly in the chat box (FezzedOne).
- Lighting is partially asynchronous (Kae).
- Various changes to the storage of directives and images in memory to greatly reduce their impact on FPS (Kae).
  - Works well when extremely long directives are used for «vanilla multiplayer-compatible» creations, like [generated](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) [clothing](https://github.com/FezzedOne/FezzedOne-Drawable-Generator).
- Client-side tile placement prediction (rewrite by Kae from StarExtensions).
  - You can also resize the placement area of tiles on the fly.
- Client- and server-side support for placing foreground tiles with a custom collision type (rewrite by Kae from StarExtensions; requires xServer or xClient on the host). Compatible with the overground placement feature of StarExtensions and OpenStarbound clients. [xWEdit](https://github.com/FezzedOne/xWEdit), a fork of WEdit with support for these features, is available; xWEdit requires xClient for full client-side functionality, but partially works with OpenStarbound clients (not StarExtensions!).
  - Additionally, objects can be placed under non-solid foreground tiles (Kae).
- Support for placing tiles in mid-air, not connected to existing ones, via an extra argument to `world.placeMaterial()` (requires *both* xClient and, in multiplayer, xServer/xClient on the host). By FezzedOne.
- Some polish to UI (FezzedOne and Kae).
- Terraria-like placement animations for objects, tiles and liquids (FezzedOne). Can be disabled with an asset mod if you don't like them.

## Mod compatibility

Read this to see if xStarbound is compatible with your mods.

**Has xStarbound support:** The following mods have special functionality that requires or is supported by xStarbound.

- [Enterable Fore Block](https://steamcommunity.com/sharedfiles/filedetails/?id=3025026792) — fully supported by xStarbound.
- [FezzedTech](https://steamcommunity.com/sharedfiles/filedetails/?id=2962923060) ([GitHub](https://github.com/FezzedOne/FezzedTech)) — requires xStarbound for full functionality, but also supports StarExtensions (with reduced functionality) and is compatible with vanilla Starbound. Is *not* compatible with OpenStarbound.
- [Tech Loadout Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2920684844) — fully supported by xStarbound.
- [Scanner Shows Printability](https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034) — fully supported by xStarbound as of v2.3.7.
- [Size of Life - Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=3218820111), [Size of Life - Vanilla Species](https://steamcommunity.com/sharedfiles/filedetails/?id=3218826863) and other mods based on the framework — xStarbound supports «nice» non-pixelated scaling as of v2.4.1.1.
- [Time Control Command](https://steamcommunity.com/sharedfiles/filedetails/?id=3256623666) ([GitHub](https://github.com/bongus-jive/TimeControlCommand)) — fully supported by xStarbound.
- [Universal Instant Crafting for All Mods](https://steamcommunity.com/sharedfiles/filedetails/?id=3251274439) — As of v2.5, fully supported by xStarbound.
- [xAdvancedChat](https://github.com/FezzedOne/xAdvancedChat) — requires xStarbound v2.3.7+. Supports most features of and is network-compatible with «upstream» [StarCustomChat](https://github.com/KrashV/StarCustomChat).
- [xSIP](https://github.com/FezzedOne/xSIP) — this Spawnable Item Pack fork's universal mod support requires xStarbound v2.5+ or OpenStarbound.
- [xWEdit](https://github.com/FezzedOne/xWEdit) — this WEdit fork requires xStarbound for full functionality, but is partially supported by OpenStarbound (no mid-air tile placement) and compatible with vanilla Starbound (with no extra functionality above WEdit).
- Mods that change the size or number of bags in the inventory or hotbar — as of v2.4, xStarbound gives these mods full compatibility with vanilla multiplayer and existing characters «out of the box».

**Compatible:** Any mod not listed in the «partially compatible» or «not compatible» category should be compatible. Major mods that have been tested to be compatible:

- [Arcana](https://steamcommunity.com/workshop/filedetails/?id=2359135864) — use [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) to satisfy this mod's «Stardust Core Lite» dependency (see that mod below for why).
- [Avali (Triage) Race Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=729558042).
- [Elithian Races Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=850109963).
- [Frackin' Universe](https://steamcommunity.com/sharedfiles/filedetails/?id=729480149) ([GitHub](https://github.com/sayterdarkwynd/FrackinUniverse)).
- [Infinite Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=1944652893) — the keybind is not supported by xStarbound solely due to an explicit StarExtensions check.
- [Maple32](https://steamcommunity.com/sharedfiles/filedetails/?id=2568667104&searchtext=maple32).
- [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034).
- [Shellguard: Starbound Expansion Remastered](https://steamcommunity.com/sharedfiles/filedetails/?id=1563376005).

> **Note:** xStarbound does not and will not support StarExtensions' «body dynamics» and text-to-speech features, and currently doesn't support StarExtensions' species-specific head rotation parameters. Details:
>
> - Armour, clothing and race mods with included SE «body dynamics» support are compatible, but the «non-jiggle» sprites will be displayed.
> - Race *and* race-modifying mods with StarExtensions head rotation parameters, such as [Nekify](https://steamcommunity.com/sharedfiles/filedetails/?id=2875605913), may have visual sprite glitches — such as Neki ears being clipped off — while xStarbound's head rotation is enabled.
> - Mods intended to patch in «body dynamics» support or StarExtensions-specific head rotation parameters for other mods simply will not work at all.
> - Race mods that support StarExtensions' text-to-speech feature will work just fine, but the text-to-speech functionality won't work.

**Partially compatible:** The following mods are only partially compatible with xStarbound:

- [1x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=1782208070) — won't do anything and is redundant.
- [3x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2681858844) — ditto.
- [4x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2870596125) — ditto.
- [Bottinator22's mods](https://steamcommunity.com/profiles/76561197964469434/myworkshopfiles/?appid=211820) — these have issues with xStarbound's Lua sandbox. You'll probably have to disable `"safeScripts"` if you use any of these mods.
- [Optimizebound](https://steamcommunity.com/sharedfiles/filedetails/?id=902555153) and its [two](https://steamcommunity.com/sharedfiles/filedetails/?id=2954344118) [counterparts](https://steamcommunity.com/sharedfiles/filedetails/?id=2954354494) — their PNG compression does not play well with xClient's hardware cursor, potentially making it invisible. Use `/cursor off` if you want to use these mods.
- [Project Knightfall](https://steamcommunity.com/sharedfiles/filedetails/?id=2010883172) ([GitHub](https://github.com/Nitrosteel/Project-Knightfall)) — some added UIs have minor issues with xStarbound's Lua sandbox.
- [Русификатор Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2980671752) — technically fully compatible, but the mod it patches isn't compatible anyway.
- [Ruler](https://steamcommunity.com/sharedfiles/filedetails/?id=2451043851) — not compatible with xStarbound's Lua sandbox. If you use this mod, you need to disable `"safeScripts"`.
- [Stardust Core](https://steamcommunity.com/sharedfiles/filedetails/?id=764887546) and [Stardust Core Lite](https://steamcommunity.com/sharedfiles/filedetails/?id=2512589532) ([GitHub](https://github.com/zetaPRIME/sb.StardustSuite)) — not compatible with xStarbound's Lua sandbox. Either disable `"safeScripts"` or use [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) instead.
- [Stardust Suite](https://github.com/zetaPRIME/sb.StardustSuite) ([GitHub](https://github.com/zetaPRIME/sb.StardustSuite)) — Ditto.
- [Starloader](https://steamcommunity.com/sharedfiles/filedetails/?id=2936533996) ([GitHub](https://github.com/Starbound-Neon/StarLoader)) — fully compatible as long as `"safeScripts"` is disabled in your `xclient.config` (but be careful with that!).
- [Unitilities | Lua Modding Library](https://steamcommunity.com/sharedfiles/filedetails/?id=2826961297) — the Hasibound-specific functionality is not supported by xStarbound.
- Other UI scaling mods — these won't do anything and are redundant. Mods that do other stuff besides scaling the UI should work.

**Not compatible:** The following mods are *NOT* compatible with xStarbound:

- [Actionbar Group Scrolling](https://steamcommunity.com/sharedfiles/filedetails/?id=3051031813) — Would work if it didn't have an explicit StarExtensions check.
- [boner guy](https://steamcommunity.com/sharedfiles/filedetails/?id=2992238651) — Would work if it didn't have an explicit StarExtensions check.
- [Futara's Dragon Engine](https://steamcommunity.com/sharedfiles/filedetails/?id=2297133082) — Causes severe server- *and* client-side entity lag on xStarbound.
- [Futara's Dragon Race](https://steamcommunity.com/sharedfiles/filedetails/?id=1958993491) — Causes a whole bunch of errors on xStarbound. Also depends on Futara's Dragon Engine and thus inherits its severe lag issue.
- [Limited Lives](https://steamcommunity.com/sharedfiles/filedetails/?id=3222951645) — would be fully supported if not for an explicit StarExtensions check.
- [Matter Manipulator Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=3266061335) ([GitHub](https://github.com/bongus-jive/mm-keybinds/tree/main)) — Not compatible because xStarbound has `root.assetSource` instead of `root.assetOrigin`.
- [More Action Bar Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2962464896) — Would work if it didn't have an explicit StarExtensions check. Use [FezzedTech](https://github.com/FezzedOne/FezzedTech) instead.
- [Quick Commands!](https://steamcommunity.com/sharedfiles/filedetails/?id=3145473452) — this mod explicitly checks for StarExtensions, so the keybinds do not work, although some of the added hidden chat commands do.
- [Remote Module](https://steamcommunity.com/sharedfiles/filedetails/?id=2943917766) — won't work and is likely to log script errors.
- [StarCustomChat](https://github.com/KrashV/StarCustomChat) — not fully compatible with xStarbound's callbacks and has serious issues with xStarbound's Lua sandbox anyway.
- [StarExtensions](https://github.com/StarExtensions/StarExtensions) — won't load on xClient and may cause crashes! However, *xServer* fully supports the server-side part of SE's «overground» tile placement feature.
- [Text to Speech Droids](https://steamcommunity.com/sharedfiles/filedetails/?id=2933125939) — won't do anything.
- [Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2916058850) — will log script errors (xStarbound has differently named callbacks) and is redundant anyway because xStarbound already fully supports this feature.
- Mods that patch in StarExtensions «body dynamics» support for other mods. These won't do anything.

## Building

This repository is already set up for easy building. Follow the appropriate instructions for your OS if listed; if your OS *isn't* listed, adjustments generally shouldn't be too complex. Note that building with Clang/LLVM is *not* properly supported, and will likely never be — expect Clang builds to be a buggy mess.

### Linux

On Linux, the xStarbound binaries are by default built against the system libraries. To build xStarbound on any reasonably up-to-date Linux install:

1. If you're on SteamOS, run `sudo steamos-readonly disable`.
2. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
3. Install CMake, Git and the required build libraries for xStarbound:
   - *Arch-based distros (CachyOS, Endeavour, etc.):* `sudo pacman -S cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland` (you may need to `-Syu` first)
   - *RPM/`yum`-based distros:* `sudo yum install cmake git ninja-build mesa mesa-libGLU libXrender libXi libxkbcommon egl-wayland`
   - *Debian/`apt`-based distros:* `sudo apt install cmake git ninja-build build-essential libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev`
   - *Gentoo:* `sudo emerge -a dev-vcs/git dev-build/cmake dev-build/ninja media-libs/mesa virtual/glu x11-misc/xcb x11-libs/libGLw x11-libs/libXrender x11-libs/libXi x11-libs/libxkbcommon gui-libs/egl-wayland`
   - *SteamOS:* `sudo steamos-readonly disable; sudo pacman -Syu cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland; sudo steamos-readonly enable`
4. If you're on SteamOS, run `sudo steamos-readonly enable`.
5. `git clone https://github.com/FezzedOne/xStarbound.git`
6. `cd xStarbound/`
7. `cmake --preset "linux-vcpkg-x86_64"`
8. `mkdir -p dist/`
9. `cp source/extern/steam/lib/linux/x86_64/libsteam_api.so dist/`
10. `cp scripts/linux/xclient.sh scripts/linux/xserver.sh scripts/linux/mod_uploader.sh dist/`
11. `scripts/linux/setup.sh 4` (increase that `4` to `8` or more if you've got a beefy system!)
12. Executables should appear in `$src/dist` if built successfully.
13. `chmod a+x dist/xclient dist/xclient.sh dist/xserver dist/xserver.sh dist/mod_uploader dist/mod_uploader.sh dist/btree_repacker`
14. `mkdir -p ${sbInstall}/xsb-linux; cp dist/* ${sbInstall}/xsb-linux/`
15. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
16. Optionally configure Steam or your other launcher to launch `${sbInstall}/xsb-linux/xclient.sh`.

> **Important:** If you're getting library linking errors while attempting to build or run xStarbound (this is likely on Debian-based distros, Slackware and CentOS due to their older libraries), you'll need to either figure out how to build xStarbound against the Steam runtime (hint: update CMake somehow!) or find a way to update your system libraries.

### Windows 10 or 11

To build and install xStarbound on Windows 10 or 11:

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/whatsnew/) and [CMake](https://cmake.org/download/). For Visual Studio, make sure to install C++ support (and optionally the game dev stuff) when the VS installer asks you what to install. For CMake, make sure you download and use the `.msi` installer for 64-bit Windows.
2. Optionally install [Git](https://git-scm.com/download/win). If using Git, go the next step; otherwise go to step 4.
3. Open up Git Bash and run `git clone https://github.com/FezzedOne/xStarbound.git`, then go to step 6.
4. Download the latest xStarbound source ZIP and extract it somewhere.
5. Go into `scripts\windows\` and double-click `build.bat`. CMake and VCPKG will take care of all build dependencies.
6.  Executables, required `.dll` libraries and the required `xsbinit.config` should appear in a new `xStarbound\dist\` folder if built successfully.
7.  Make a new `xsb-win64\` folder in your Starbound install folder, and copy or move the files in `xStarbound\dist\` to it.
8.  Make a new `xab-assets\` folder in your Starbound install folder, and copy the `assets\xSBassets` folder into that folder. For correct installation, you should have an `xSBassets\` folder *inside* `xsb-assets\`.
9.  Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

> **Building on older Windows versions:** Building on earlier versions of Windows is *not* recommended, although it *should* still be possible to build xStarbound on Windows 7, 8 or 8.1 if you can get VS 2022 installed.

### Other OSes

The basic process for building on other OSes:

1. Install CMake, a C++ compiler toolchain and (optionally) Git.
   - On *macOS 10.11+*, you should use [Homebrew](https://brew.sh/) to install CMake, Git and the needed build dependencies on your system. Manual installation of dependencies via wizards or drag-and-drop is likely to cause build issues or be a pain to work with.
   - If you're targeting *older versions of macOS / OS X*, look into [Tigerbrew](https://github.com/mistydemeo/tigerbrew). Expect to do more tinkering and/or manual installation of build dependencies.
   - Most *BSDs* and other reasonably up-to-date *\*nix OSes* (like Haiku) should have the necessary packages in their ports system or equivalent. Make sure to disable Steam and Discord integrations.
   - For *Windows XP* or *Vista* targets, your best bet is to use Visual Studio 2017 or 2019 on a newer version of Windows or on WINE with the `v141_xp` toolchain (it doesn't work on VS 2022!). Make sure to disable Steam and Discord integrations.
   - Targetting *older Windows versions*, *pre-OSX Mac OS*, *Android*, the *Switch* or any other niche device or OS is left as an exercise for the reader.
2. If you can, install any dependencies needed to build SDL2 and GLEW on your OS. If you can't, hopefully VCPKG runs on your build OS and/or has packages for your target OS. If not, get ready for a lot of tinkering.
3. If you're not using Git or don't have it installed, download the latest xStarbound source ZIP, extract it somewhere and go to step 5. Otherwise go to the next step.
4. If you're using Git, run `git clone https://github.com/FezzedOne/xStarbound.git` in a terminal or command prompt, or use a graphical Git utility to download the repo.
5. In the xStarbound directory, run `cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/`. On some OSes, you may need to add the full path to your CMake executable to the beginning of the command. If necessary, add `-DCMAKE_C_COMPILER=<path to C++ compiler> -DCMAKE_CXX_COMPILER=<path to C++ compiler>`. Note that a CMake build preset already exists for modern macOS; consider using that if you have build issues.

## Discord

For support, suggestions or just plain old chit-chat, check out the [xStarbound Discord server](https://discord.gg/GJ5RTkyFCX).
