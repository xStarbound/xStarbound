# xSB-2

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xSB-2. Base game assets are not provided for obvious reasons.

Compiled builds for Linux, Windows and macOS should be available in the usual place on this repository.

## Changes

- Several new commands (by FezzedOne)! Type `/xsb` for info on the new client-side commands, or `/help` (on xServer, an xcClient host or in single-player on xClient) to see the new server-side ones.
- Nicer (and optimised) non-pixelated humanoid tech and status effect scaling for players and NPCs (reimplementation by FezzedOne).
- The UI scale can now be adjusted in the graphics settings dialogue, complete with configurable keybinds and support for fractional scales (FezzedOne). There are also keybinds for changing the in-game camera zoom (Kae). Both the UI scale and zoom level are scriptable (FezzedOne). UI scaling mods are no longer needed (and in fact no longer do anything) in xSB-2!
- Inventory and action bar expansion (or reduction) mods are now fully compatible with vanilla multiplayer with no changes needed on the mod's part. Additionally, these mods can now be safely added or removed without item loss or crashes as long as characters are loaded in xSB-2. Added by WasabiRaptor and FezzedOne.
  - Loading a character after changes to inventory or action bar mods will drop any items that no longer fit on the ground beside the player (which will be picked up if there still is inventory space), instead of showing an error dialogue.
  - *Warning for users of vanilla clients and other client mods:* If you load any characters that have their inventories resized but haven't yet dropped overflowed items on a non-xSB-2 client — this can happen if you open xSB-2 after changing mods, but don't actually load some characters before switching to another client — you may lose items, so load your characters in xSB-2 and save your items first!
  - The networked inventory and action bar config can be configured separately with a patch to `$assets/player.config`; see [`$src/assets/xSBassets/player.config.patch`](https://github.com/FezzedOne/xSB-2/blob/main/assets/xSBassets/player.config.patch) for the new parameters. Such a patch mod is *required* for an xClient client to join a non-xServer server with inventory or action bar mods installed, but allows joining the server with mismatched mods (assuming mismatched assets are allowed).
- Anything that the game converts to a Perfectly Generic Item now has its parameters saved in the item and will be restored once any missing mods are reinstalled (WasabiRaptor and FezzedOne). Requires xServer (or xClient on the host) for server-side items (such as those in containers on worlds, even shipworlds!) and xClient for single-player and client-side items (those in the player's inventory).
- Modded techs and status effects no longer cause crashes to the menu when the offending mod is removed (Kae and WasabiRaptor).
- Scriptable shader and lighting parameters are supported (FezzedOne).
- You can now make `.patch` files that are just merged in, early-beta-style (Kae). That's why the patch files in `assets/xSBassets` are unusually simple. In addition, a new `"find"` parameter is supported for `"remove"` and `"replace"` operations where the `"path"` is to a JSON array (FezzedOne) — if `"find"` is present, only the first array value exactly matching the value in `"find"` is removed or replaced by `"value"`, not the entire array.
- Almost all Lua callbacks from the original xSB (by FezzedOne), `input` callbacks (by Kae), plus some extra `player`, `chat`, `interface` and `clipboard` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods (FezzedOne). The `setSpecies` and `setIdentity` callbacks will not let you switch to a nonexistent species (FezzedOne). Documentation has yet to be updated.
- Various crash fixes (FezzedOne and Kae).
- Character swapping (rewrite by Kae from StarExtensions): `/swap <name>` (case-insensitive substring matching) and `/swapuuid <uuid>` (requires a UUID; use the one in the player file name).
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
- Client- and server-side support for placing foreground tiles with a custom collision type (rewrite by Kae from StarExtensions; requires xServer or xClient on the host) and, via `world.placeMaterial()`, placing tiles not connected to existing ones (FezzedOne; requires xServer or xClient on the host). Compatible with the overground placement feature of StarExtensions and OpenStarbound clients. [xWEdit](https://github.com/FezzedOne/xWEdit), a fork of WEdit with support for these features, is available; xWEdit requires xClient for full client-side functionality, but partially works with OpenStarbound clients (not StarExtensions!).
  - Additionally, objects can be placed under non-solid foreground tiles (Kae).
- Some polish to UI (FezzedOne and Kae).
- Terraria-like placement animations for objects, tiles and liquids (FezzedOne). Can be disabled with an asset mod if you don't like them.

## Mod compatibility

Read this to see if xSB-2 is compatible with your mods.

**Has xSB-2 support:** The following mods have special functionality that requires or is supported by xSB-2.

- [Enterable Fore Block](https://steamcommunity.com/sharedfiles/filedetails/?id=3025026792) — fully supported by xSB-2.
- [FezzedTech](https://steamcommunity.com/sharedfiles/filedetails/?id=2962923060) ([GitHub](https://github.com/FezzedOne/FezzedTech)) — requires xSB-2 for full functionality, but also supports OpenStarbound and StarExtensions (with reduced functionality) and is compatible with vanilla Starbound.
- [Tech Loadout Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2920684844) — fully supported by xSB-2.
- [Scanner Shows Printability](https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034) — fully supported by xSB-2 as of v2.3.7.
- [Size of Life - Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=3218820111) and [Size of Life - Vanilla Species](https://steamcommunity.com/sharedfiles/filedetails/?id=3218826863) — xSB-2 fully supports non-pixelated scaling as of v2.4.1.1.
- [xAdvancedChat](https://github.com/FezzedOne/xAdvancedChat) — requires xSB-2 v2.3.7+. Supports most features of and is fully network-compatible with «upstream» [StarCustomChat](https://github.com/KrashV/StarCustomChat).
- [xWEdit](https://github.com/FezzedOne/xWEdit) — this WEdit fork requires xSB-2 for full functionality, but is partially supported by OpenStarbound (no mid-air tile placement) and compatible with vanilla Starbound (with no extra functionality above WEdit).
- Mods that change the size or number of bags in the inventory or hotbar — as of v2.4, xSB-2 gives these mods full compatibility with vanilla multiplayer and existing characters «out of the box».

**Compatible:** Any mod not listed in the «partially compatible» or «not compatible» category should be compatible. Major mods that have been tested to be compatible:

- [Arcana](https://steamcommunity.com/workshop/filedetails/?id=2359135864).
- [Avali (Triage) Race Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=729558042).
- [Elithian Races Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=850109963).
- [Frackin' Universe](https://steamcommunity.com/sharedfiles/filedetails/?id=729480149) ([GitHub](https://github.com/sayterdarkwynd/FrackinUniverse)).
- [Infinite Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=1944652893) — the keybind is not supported by xSB-2 solely due to an explicit StarExtensions check.
- [Maple32](https://steamcommunity.com/sharedfiles/filedetails/?id=2568667104&searchtext=maple32).
- [Project Knightfall](https://steamcommunity.com/sharedfiles/filedetails/?id=2010883172).
- [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034).
- [Ruler](https://steamcommunity.com/sharedfiles/filedetails/?id=2451043851) — the keybind is not supported by xSB-2 solely due to an explicit StarExtensions check.
- [Shellguard: Starbound Expansion Remastered](https://steamcommunity.com/sharedfiles/filedetails/?id=1563376005).
- [Stardust Core Lite](https://steamcommunity.com/sharedfiles/filedetails/?id=2512589532) — but compatibility may change in future versions!
- [Stardust Suite](https://github.com/zetaPRIME/sb.StardustSuite) — but compatibility may change in future versions!

> **Note:** xSB-2 does not and will not support StarExtensions' «body dynamics» and text-to-speech features, and currently doesn't support StarExtensions' species-specific head rotation parameters. Details:
>
> - Armour, clothing and race mods with included SE «body dynamics» support are compatible, but the «non-jiggle» sprites will be displayed.
> - Race *and* race-modifying mods with StarExtensions head rotation parameters, such as [Nekify](https://steamcommunity.com/sharedfiles/filedetails/?id=2875605913), may have visual sprite glitches — such as Neki ears being clipped off — while xSB-2's head rotation is enabled.
> - Mods intended to patch in «body dynamics» support or StarExtensions-specific head rotation parameters for other mods simply will not work at all.
> - Race mods that support StarExtensions' text-to-speech feature will work just fine, but the text-to-speech functionality won't work.

**Partially compatible:** The following mods are only partially compatible with xSB-2:

- [1x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=1782208070) — won't do anything and is redundant.
- [3x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2681858844) — ditto.
- [4x UI scaling](https://steamcommunity.com/sharedfiles/filedetails/?id=2870596125) — ditto.
- [Русификатор Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2980671752) — technically fully compatible, but the mod it patches isn't compatible anyway.
- [Starloader](https://steamcommunity.com/sharedfiles/filedetails/?id=2936533996) ([GitHub](https://github.com/Starbound-Neon/StarLoader)) — fully compatible as long as `"safeScripts"` is disabled in your `xclient.config` (but be careful with that!).
- [Unitilities | Lua Modding Library](https://steamcommunity.com/sharedfiles/filedetails/?id=2826961297) — the Hasibound-specific functionality is not supported by xSB-2.
- Other UI scaling mods — these won't do anything and are redundant. Mods that do other stuff besides scaling the UI should work.

**Not compatible:** The following mods are *NOT* compatible with xSB-2:

- [Actionbar Group Scrolling](https://steamcommunity.com/sharedfiles/filedetails/?id=3051031813) — Would work if it didn't have an explicit StarExtensions check.
- [boner guy](https://steamcommunity.com/sharedfiles/filedetails/?id=2992238651) — Would work if it didn't have an explicit StarExtensions check.
- [More Action Bar Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2962464896) — Would work if it didn't have an explicit StarExtensions check. Use [FezzedTech](https://github.com/FezzedOne/FezzedTech) instead.
- [Quick Commands!](https://steamcommunity.com/sharedfiles/filedetails/?id=3145473452) — this mod explicitly checks for StarExtensions, so the keybinds do not work, although some of the added hidden chat commands do.
- [Remote Module](https://steamcommunity.com/sharedfiles/filedetails/?id=2943917766) — won't work and is likely to log script errors.
- [StarCustomChat](https://github.com/KrashV/StarCustomChat) — use the fully network-compatible [xAdvancedChat](https://github.com/FezzedOne/xAdvancedChat) fork instead.
- [StarExtensions](https://github.com/StarExtensions/StarExtensions) — won't load on xClient and may cause crashes! However, *xServer* fully supports the server-side part of SE's «overground» tile placement feature.
- [Text to Speech Droids](https://steamcommunity.com/sharedfiles/filedetails/?id=2933125939) — won't do anything.
- [Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2916058850) — will log script errors (xSB-2 has differently named callbacks) and is redundant anyway because xSB-2 already fully supports this feature.
- Mods that patch in StarExtensions «body dynamics» support for other mods. These won't do anything.

## Building

This repository is already set up for easy building. Follow the appropriate instructions for your OS if listed; if your OS *isn't* listed, adjustments generally shouldn't be too complex. Note that building with Clang/LLVM is *not* properly supported, and will likely never be — expect Clang builds to be a buggy mess.

### Linux

The xSB-2 binaries can be built against either the Steam Runtime or the native system libraries; the `mod_uploader` may optionally be built separately. All prebuilt Linux binaries starting with v2.3.6.1r1 on this repo have been built against the Steam Runtime to ensure cross-distro compatibility.

**With Steam Runtime:** To build against the Steam Runtime:

1. Install Toolbox and Git — your distro's packages should be called `toolbox` and `git`, respectively.
2. `toolbox create -i registry.gitlab.steamos.cloud/steamrt/scout/sdk scout`
3. `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`
4. `toolbox enter scout`
5. *In the Toolbox shell:* `cd xSB-2/`
6. *In the Toolbox shell:* `scripts/linux/setup-runtime.sh 4` (increase that `4` to `8` or more if you've got a beefy system!)
7. `mkdir -p ${sbInstall}/xsb-linux; cp dist/* ${sbInstall}/xsb-linux/`
8. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
9.  Optionally configure Steam or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `${sbInstall}/xsb-linux/xclient`.

> **Note:** All extra libraries required by xSB-2 should already be set up in the repo — there's no need for `apt-get` commands in the Toolbox environment.

**With system libraries:** *Not recommended on non-Arch-based distros!* To build against system libraries on any reasonably up-to-date Linux distro:

1. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
2. Install CMake and Git:
   - *Arch-based distros:* `sudo pacman -S cmake git` (you may need to `-Syu` first)
   - *RPM/`yum`-based distros:* `sudo yum install cmake git`
   - *Debian/`apt-get`-based distros:* `sudo apt-get install cmake git` (on Mint, replace `apt-get` with `apt`)
   - *Gentoo:* `sudo emerge -a dev-vcs/git dev-build/cmake`
   - *SteamOS:* `sudo steamos-readonly disable; sudo pacman -Syu cmake git; sudo steamos-readonly enable`
3. `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`
4. `cd xSB-2/`
5. `scripts/linux/setup.sh 4` (increase that `4` to `8` or more if you've got a beefy system!)
6. Executables, required `.so` libaries and the required `sbinit.config` should appear in `$src/dist` if built successfully.
7. `mkdir -p ${sbInstall}/xsb-linux; cp dist/* ${sbInstall}/xsb-linux/`
8. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
9.  Optionally configure Steam or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `${sbInstall}/xsb-linux/xclient`.

> **Important:** If you're getting library linking errors while attempting to build or run xSB-2 (this is likely on Debian-based distros, Slackware and CentOS due to their older libraries), you'll need to either build xSB-2 against the Steam runtime or find a way to update your system libraries.

**Building the mod uploader:** The Linux `mod_uploader` (needed to upload mods to the Steam Workshop) must be built separately and *cannot* be built in the Steam runtime. You *must* own Starbound on Steam in order to be able to use the `mod_uploader`. To build the `mod_uploader`:

1. Install CMake and Git; see above for package names on popular distros.
2. Install Steam if you don't already have it installed; see [these instructions](https://www.xda-developers.com/how-run-steam-linux/) if you don't know how.
3. Install `libpng` 16.x and the Qt5 libraries and headers:
   - *Arch-based distros:* `sudo pacman -S qt5-base libpng` (you may need to `-Syu` first)
   - *RPM/`yum`-based distros:* `sudo yum install qt5-qtbase qt5-qtbase-devel libpng`
   - *Debian/`apt-get`-based distros:* `sudo apt-get install qtbase5-dev libpng16-16` (on Mint, replace `apt-get` with `apt`)
   - *Gentoo:* `sudo emerge -a @qt5-essentials media-libs/libpng`
   - *SteamOS:* `sudo steamos-readonly disable; sudo pacman -Syu qt5-base libpng; sudo steamos-readonly enable`
4. `cd path/to/xSB-2/`
5. `scripts/linux/setup-qt.sh 4` (increase that `4` to `8` or more if you've got a beefy system!)
6. Once built, `cp` the `mod_uploader` in `$src/dist` to anywhere convenient, optionally renaming it.

To use the `mod_uploader`, start Steam and then start the `mod_uploader` binary manually — Linux Steam does not have an option to use it through the game library. Once started, you can upload mods normally.

> **Note:** If you get a «`libpng` not found» linker error while building, change the `libpng` path in `$src/scripts/linux/setup-qt.sh` to wherever `libpng16.so.16` is installed on your system.

### SteamOS

To build on SteamOS:

1. Run the following commands:

    ```sh
    sudo steamos-readonly disable
    sudo pacman -S toolbox git
    sudo steamos-readonly enable
    ```

2. Follow the Steam Runtime instructions starting at step 3.

You will need to re-run the commands in step 1 every time you update SteamOS (and want to rebuild xSB-2).

### Windows

To build and install on Windows 10 or 11:

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/whatsnew/) and [CMake](https://cmake.org/download/). For Visual Studio, make sure to install C++ support (and optionally the game dev stuff) when the VS installer asks you what to install. For CMake, make sure you download and use the `.msi` installer for 64-bit Windows
2. Optionally install [Git](https://git-scm.com/download/win). If using Git, go the next step; otherwise go to step 4.
3. Open up Git Bash and run `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`, then go to step 6.
4. Download the latest xSB-2 source ZIP and extract it somewhere.
5. Download the latest [Opus source ZIP](https://github.com/xiph/opus/releases), extract it, and put the `opus\` folder in `source\extern\`
6. Go into `scripts\windows\` and double-click `setup.bat`.
7. Wait for that batch file to finish, go up two folders, open up `build\` and double-click `ALL_BUILD.vcxproj`.
8. Select **Build → Build Solution** in Visual Studio.
9.  Executables, required `.dll` libraries and the required `sbinit.config` should appear in a new `xSB-2\dist\` folder if built successfully.
10. Make a new `xsb-win64\` folder in your Starbound install folder, and copy or move the `.exe`s and `.dll`s to it.
11. Make a new `xab-assets\` folder in your Starbound install folder, and copy the `assets\xSBassets` folder into that folder.
12. Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

Building on earlier versions of Windows is not recommended, although it *should* still be possible to build xSB-2 on Windows 7, 8 or 8.1 if you can get VS 2022 installed.

### macOS

To build and install on macOS 10.15 or later:

1. Run `xcode-select --install` in a terminal if you don't already have the Xcode CLI tools installed; if you have issues with the following steps, update macOS afterward.
2. Download and install [CMake](https://cmake.org/download/) and [Homebrew](https://brew.sh/).
3. Download and install [SDL2](https://github.com/libsdl-org/SDL/releases/tag/release-2.30.0) — use the DMG with the framework!
4. Run the following terminal commands: `brew install git glew libvorbis lzlib libpng freetype`
5. `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`
6. `cd xSB-2`
7. `scripts/osx/setup.command; scripts/osx/build.command`
8. `cp dist/xclient macos/xClient.app/Contents/MacOS/`
9. Copy everything in `xSB-2/dist` except `xclient` to a new `xsb-osx` folder in your Starbound install folder, then copy `xClient.app` («xClient» in Finder) from `xSB-2/macos` to the new `xsb-osx` folder.
10. Copy `xSB-2/assets/xSBassets` to a new `xsb-assets` folder in your Starbound install folder.
11. Optionally configure Steam or GoG to launch `xsb-osx/xClient.app`.

### Cross-compilation from Linux to Windows

To cross-compile from Linux to Windows:

1. Install CMake, WINE, MinGW-w64 and Git (if not already preinstalled).
  - **Arch-/Debian-based distros (`apt`/`pacman`):** Install `cmake`, `wine`, `mingw-w64` and `git`.
  - **Fedora-based distros (`yum`):** Install `cmake`, `wine`, `mingw64-\*` and `git`.
2. Install or build the MinGW versions of Freetype (using `--without-harfbuzz`), GLEW, ZLib, `libpng`, `libogg`, `libvorbis` and SDL2.
  - On the AUR, these are `mingw-w64-freetype2-bootstrap`, `mingw-w64-zlib`, `mingw-w64-glew`, `mingw-w64-zlib`, `mingw-w64-libpng`, `mingw-w64-libogg`, `mingw-w64-libvorbis` and `mingw-w64-sdl2`, respectively.
  - For Arch users (*not* derivatives), there is a [binary repo](https://martchus.no-ip.biz/repo/arch/ownstuff) for these libraries, but you should still install `mingw-w64-freetype2-bootstrap` from the AUR.
3. `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`
4. `cd xSB-2/`
5. `scripts/mingw/setup.sh 4` (increase that `4` to `8` or more if you've got a beefy system!)
6. Executables, required `.dll` libaries and the required `sbinit.config` should appear in `$src/dist-windows` if built successfully. Note that the Discord library is differently named due to an idiosyncrasy with the linker; do not rename it back.
7. `mkdir -p ${sbInstall}/xsb-win64; cp dist-windows/* ${sbInstall}/xsb-win64/`
8. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
9. Optionally configure Steam or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `${sbInstall}/xsb-win64/xclient.exe` through WINE/Proton (or on your Windows install).

> **Note:** Gentoo users will need to compile MinGW with `_GLIBCXX_USE_CXX11_ABI` enabled (`-D_GLIBCXX_USE_CXX11_ABI=1`) to avoid linker errors.

## Discord

For support, suggestions or just plain old chit-chat, check out the [xSB-2 Discord server](https://discord.gg/GJ5RTkyFCX).
