# xStarbound

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xStarbound. Base game assets are not provided for obvious reasons.

Compiled builds for Linux and Windows should be available in the usual place on this repository.

## Changes

- Several new commands (by @fezzedone)! Type `/xclient` for info on the new client-side commands, or `/help` (on xServer, an xClient host or in single-player on xClient) to see the new server-side ones.
- Nicer (and optimised) non-pixelated humanoid tech and status effect scaling for players and NPCs (reimplementation by @fezzedone).
- Now runs [Pluto](https://pluto-lang.org/), a fork of Lua 5.4!
- Full Lua sandboxing when `"safeScripts"` is enabled! By @fezzedone.
  - To replace the old, potentially crash-prone sandbox-breaking code used by certain mods, new Lua callbacks for safely saving and reading variables in global variable tables with the same expected cross-context scopes.
  - **Note:** This causes some mod compatibility issues; see below for affected mods.
- Full, up-to-date Lua API documentation. (Aside from a lot of engine calls into Lua scripts.)
- Control multiple characters on a single client! Is fully multiplayer-compatible. By @fezzedone. Replaces OpenStarbound's character swapping feature.
  - `/add` and `/adduuid`: Loads and adds a player character from your saves.
  - `/swap` and `/swapuuid`: Swaps to a different character. If the character isn't loaded, replaces your current character.
  - `/remove` and `/removeuuid`: Removes a character you're not currently controlling.
  - There are some game balance restrictions — dead characters won't respawn until you beam to your ship. The restrictions can be disabled via the Lua API on a per-character basis.
- xStarbound now has OpenStarbound's world file flattening and bloat fixes! By @novaenia.
- xStarbound automatically repacks shipworld and celestial world files when loading them, saving you quite a bit of disk space and, for xClient, reducing server lag caused by shipworlds. By @fezzedone.
  - Shipworld repacking is client-side; celestial world repacking is server-side.
  - Disable this automatic repacking by adding `"disableRepacking": true` to `xclient.config` or `xserver.config`.
- Additional Lua callbacks to make player characters fully scriptable, just like NPCs!
- The UI scale can now be adjusted in the graphics settings dialogue, complete with configurable keybinds and support for fractional scales (@fezzedone). There are also keybinds for changing the in-game camera zoom (@novaenia). Both the UI scale and zoom level are scriptable (@fezzedone). UI scaling mods are no longer needed (and in fact no longer do anything) in xStarbound!
- Chat message history is now saved to `messages.json` in your storage directory instead of being reset on every disconnection. Use the new `/clear` command on xClient to clear the chat history instead.
- Inventory and action bar expansion (or reduction) mods are now fully compatible with vanilla multiplayer with no changes needed on the mod's part. Additionally, these mods can now be safely added or removed without item loss or crashes as long as characters are loaded in xStarbound. Added by @WasabiRaptor and @fezzedone.
  - Loading a character after changes to inventory or action bar mods will drop any items that no longer fit on the ground beside the player (which will be picked up if there still is inventory space), instead of showing an error dialogue.
  - *Warning for users of vanilla clients and other client mods:* If you load any characters that have their inventories resized but haven't yet dropped overflowed items on a non-xStarbound client — this can happen if you open xStarbound after changing mods, but don't actually load some characters before switching to another client — you may lose items, so load your characters in xStarbound and save your items first!
  - The networked inventory and action bar config can be configured separately with a patch to `$assets/player.config`; see [`$src/assets/xSBassets/player.config.patch`](https://github.com/fezzedone/xStarbound/blob/main/assets/xSBassets/player.config.patch) for the new parameters. Such a patch mod is *required* for an xClient client to join a non-xServer server with inventory or action bar mods installed, but allows joining the server with mismatched mods (assuming mismatched assets are allowed).
- Anything that the game converts to a Perfectly Generic Item now has its parameters saved in the item and will be restored once any missing mods are reinstalled (@WasabiRaptor and @fezzedone). Requires xServer (or xClient on the host) for server-side items (such as those in containers on worlds, even shipworlds!) and xClient for single-player and client-side items (those in the player's inventory).
- Supports scriptable asset preprocessing. By @novaenia; fixed and greatly enhanced by @fezzedone.
- Modded techs and status effects no longer cause crashes to the menu when the offending mod is removed (@WasabiRaptor and @novaenia).
- Scriptable shader and lighting parameters are supported (@fezzedone).
- You can now make `.patch` files that are just merged in, early-beta-style (@novaenia). That's why the patch files in `assets/xSBassets` are unusually simple. All of OpenStarbound's JSON patch parameters are also supported.
- Almost all Lua callbacks from the original xSB (by @fezzedone), `input` callbacks (by @novaenia), plus some extra `player`, `chat`, `interface` and `clipboard` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods (@fezzedone).
- Various crash fixes (@fezzedone and @novaenia).
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
- Support for placing tiles in mid-air, not connected to existing ones, via an extra argument to `world.placeMaterial()` (requires *both* xClient and, in multiplayer, xServer/xClient on the host). By @fezzedone.
- Some polish to UI (@fezzedone and @novaenia).
- Added Wayland support (@emanueljg).
- Terraria-like placement animations for objects, tiles and liquids (@fezzedone). Can be disabled with an asset mod if you don't like them.
- Nix build & install support (emanueljg). See [Nix](#nix).

## Mod compatibility

Read this to see if xStarbound is compatible with your mods.

**Has xStarbound support:** The following mods have special functionality that requires or is supported by xStarbound.

- [Dump IDs to log](https://steamcommunity.com/sharedfiles/filedetails/?id=3333016442&searchtext=) ([GitHub](https://github.com/bongus-jive/dump-ids)) — Fully supported by xStarbound as of v3.1.5r1.
- [Enterable Fore Block](https://steamcommunity.com/sharedfiles/filedetails/?id=3025026792) — fully supported by xStarbound.
- [FezzedTech](https://steamcommunity.com/sharedfiles/filedetails/?id=2962923060) ([GitHub](https://github.com/fezzedone/FezzedTech)) — requires xStarbound for full functionality, but also supports StarExtensions (with reduced functionality) and is compatible with vanilla Starbound. Is *not* compatible with OpenStarbound.
- [Scanner Shows Printability](https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034) — fully supported by xStarbound as of v2.3.7.
- [Size of Life - Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=3218820111), [Size of Life - Vanilla Species](https://steamcommunity.com/sharedfiles/filedetails/?id=3218826863) and other mods based on the framework — xStarbound supports «nice» non-pixelated scaling as of v2.4.1.1.
- [Tech Loadout Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2920684844) — fully supported by xStarbound.
- [Time Control Command](https://steamcommunity.com/sharedfiles/filedetails/?id=3256623666) ([GitHub](https://github.com/bongus-jive/TimeControlCommand)) — fully supported by xStarbound.
- [Universal Instant Crafting for All Mods](https://steamcommunity.com/sharedfiles/filedetails/?id=3251274439) — As of v2.5, fully supported by xStarbound.
- [xAdvancedChat](https://github.com/fezzedone/xAdvancedChat) — requires xStarbound v2.3.7+. Supports most features of and is network-compatible with «upstream» [StarCustomChat](https://github.com/KrashV/StarCustomChat).
- [xSIP](https://github.com/fezzedone/xSIP) — this Spawnable Item Pack fork's universal mod support requires xStarbound v2.5+ or OpenStarbound.
- [xWEdit](https://github.com/fezzedone/xWEdit) — this WEdit fork requires xStarbound for full functionality, but is partially supported by OpenStarbound (no mid-air tile placement) and compatible with vanilla Starbound (with no extra functionality above WEdit).
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
- [Save Inventory Position](https://steamcommunity.com/sharedfiles/filedetails/?id=3331093074) ([GitHub](https://github.com/bongus-jive/save-inventory-position)) — supported by xStarbound as long as you turn off `"safeScripts"` and *don't* use `/swap` — this will cause an immediate crash!
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
- [More Action Bar Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2962464896) — Would work if it didn't have an explicit StarExtensions check. Use [FezzedTech](https://github.com/fezzedone/FezzedTech) instead.
- [Quick Commands!](https://steamcommunity.com/sharedfiles/filedetails/?id=3145473452) — this mod explicitly checks for StarExtensions, so the keybinds do not work, although some of the added hidden chat commands do.
- [Remote Module](https://steamcommunity.com/sharedfiles/filedetails/?id=2943917766) — won't work and is likely to log script errors.
- [StarCustomChat](https://github.com/KrashV/StarCustomChat) — not fully compatible with xStarbound's callbacks and has serious issues with xStarbound's Lua sandbox anyway.
- [StarExtensions](https://github.com/StarExtensions/StarExtensions) — won't load on xClient and may cause crashes! However, *xServer* fully supports the server-side part of SE's «overground» tile placement feature.
- [Text to Speech Droids](https://steamcommunity.com/sharedfiles/filedetails/?id=2933125939) — won't do anything.
- [Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2916058850) — will log script errors (xStarbound has differently named callbacks) and is redundant anyway because xStarbound already fully supports this feature.
- Mods that patch in StarExtensions «body dynamics» support for other mods. These won't do anything.

# Nix

See [xStarbound/nix](nix).

## Building

This repository is already set up for easy building (mostly @kblaschke's work, with an updated Windows build script and some additional fixes by @fezzedone). Follow the appropriate instructions for your OS if listed; if your OS *isn't* listed, adjustments generally shouldn't be too complex.

### Linux

On Linux, the xStarbound binaries are by default built against the system libraries. To build xStarbound on any reasonably up-to-date Linux install (assuming `bash` or `zsh`):

Wayland should work OOTB if you're using the `xclient.sh`. If you're running the binary yourself, you can enable this support yourself with `SDL_VIDEODRIVER="wayland"`.

1. If you're on SteamOS, run `sudo steamos-readonly disable`.
2. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
3. Install CMake, Git and the required build libraries for xStarbound:
   - *Arch-based distros (CachyOS, Endeavour, etc.):* `sudo pacman -S cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland qt6-svg qt6-base sdl2 libpng freetype2 libvorbis opus glew` (you may need to `-Syu` first)
   - *RPM/`dnf`-based distros:* `sudo dnf install gcc-c++ cmake git ninja-build mesa-libGLU libXrender libXi libxkbcommon egl-wayland qt6-qtbase qt6-qtsvg SDL2-devel libpng-devel freetype-devel libvorbis-devel opus-devel glew-devel` (yes, Fedora doesn't come with a C++ compiler preinstalled!)
   - *Debian/`apt`-based distros:* `sudo apt install cmake git ninja-build build-essential libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev qt6-base qt6-svg libsdl2-dev libpng-dev libfreetype6-dev libvorbis-dev libopus-dev libglew-dev`
   - *Gentoo:* `sudo emerge -a dev-vcs/git dev-build/cmake dev-build/ninja media-libs/mesa virtual/glu x11-misc/xcb x11-libs/libGLw x11-libs/libXrender x11-libs/libXi x11-libs/libxkbcommon gui-libs/egl-wayland media-libs/libsdl2 dev-qt/qtbase dev-qt/qtsvg media-libs/freetype media-libs/libvorbis media-libs/libpng media-libs/opus media-libs/glew`
   - *SteamOS:* `sudo steamos-readonly disable; sudo pacman -Syu cmake git ninja mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland qt6-svg qt6-base sdl2 libpng freetype2 libvorbis opus; sudo steamos-readonly enable`
4. `git clone --recursive https://github.com/xStarbound/xStarbound.git` (`--recursive` is needed to clone the Opus sources in `$src/extern/opus`, which may be necessary on some configurations)
5. `cd xStarbound/`
6. `CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake -DCMAKE_BUILD_TYPE=Release -DSTAR_ENABLE_STEAM_INTEGRATION=ON -DPACKAGE_XSB_ASSETS=ON -S . -B build/ -G Ninja`
7. `cmake --build build/`
8. `cmake --install build/ --prefix ${sbInstall}/` (replace `${sbInstall}` with the path to your Starbound install)
9. `cp scripts/linux/{xclient,xserver,mod_uploader}.sh ${sbInstall}/linux/`
10. Run `./xclient.sh` to play or `./xserver.sh` to host the server! Optionally configure Steam or your other launcher to launch `${sbInstall}/linux/xclient.sh`.

> **Important:** If you're getting library linking errors while attempting to build or run xStarbound (this is likely on Debian-based distros, Slackware and CentOS due to their older libraries) or your distro is old enough to still use `yum` or `apt-get`, you'll need to either build a statically linked version of xStarbound (see below), figure out how to build xStarbound against the Steam runtime (hint: update the runtime's CMake somehow!) or find a way to update your system libraries.

#### Statically linked builds

To build a statically linked version of xStarbound (assuming `bash` or `zsh`):

> **Important:** It is *highly* recommended you use an older LTS distribution such as Debian 11 or 12, or Ubuntu 20.x or 22.x, in order to guarantee portability. You can use a Docker, Toolbox or Podman container for this. If the chosen distro does not have CMake 3.25+ available in its repositories, download the latest `tar.gz` archive from [CMake's website](https://cmake.org/download/), extract it somewhere, and add its `bin/` directory to your `$PATH` *instead* of installing it via the package manager.

1. If you're on SteamOS, run `sudo steamos-readonly disable`.
2. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's «base development» package.
3. Install CMake, Git and the required build libraries for xStarbound:
   - *Arch-based distros (CachyOS, Endeavour, etc.):* `sudo pacman -S cmake git ninja patchelf mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland` (you may need to `-Syu` first)
   - *RPM/`dnf`-based distros:* `sudo dnf install gcc-c++ cmake git ninja-build patchelf mesa-libGLU libXrender libXi libxkbcommon egl-wayland xcb* libX11-xcb` (yes, Fedora doesn't come with a compiler preinstalled!)
   - *Debian/`apt`-based distros:* `sudo apt install cmake git ninja-build patchelf build-essential libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev libxcb-*`
   - *Gentoo:* `sudo emerge -a dev-vcs/git dev-build/cmake dev-build/ninja dev-util/patchelf media-libs/mesa virtual/glu x11-misc/xcb x11-libs/libGLw x11-libs/libXrender x11-libs/libXi x11-libs/libxkbcommon gui-libs/egl-wayland`
   - *SteamOS:* `sudo steamos-readonly disable; sudo pacman -Syu cmake git ninja patchelf mesa libx11 glu libxcb libxrender libxi libxkbcommon libxkbcommon-x11 egl-wayland; sudo steamos-readonly enable`
4. `mkdir -p ~/.local/opt; git clone https://github.com/microsoft/vcpkg.git ~/.local/opt/vcpkg`
5. `cd ~/.local/opt/vcpkg; ./bootstrap-vcpkg.sh -disableMetrics` (yes, VCPKG sends telemetry by default)
6. `cd $devDirectory` (where `$devDirectory` is the folder you want to put the xStarbound source in)
7. `git clone --recursive https://github.com/xStarbound/xStarbound.git` (`--recursive` is needed to clone the Opus sources in `$src/extern/opus`, which may be necessary on some configurations)
8. `cd xStarbound/; export VCPKG_ROOT="${HOME}/.local/opt/vcpkg"; export PATH="${VCPKG_ROOT}:${PATH}"`
9.  `CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake --build cmake-build-linux-x86_64/ --preset "linux-vcpkg-x86_64-release" -G Ninja`
10. `cmake --build build/`
11. `cmake --install build/ --prefix ${sbInstall}/` (replace `${sbInstall}` with the path to your Starbound install)
12. `cp scripts/linux/{xclient-static,xserver,mod_uploader}.sh ${sbInstall}/linux/`
13. `mv ${sbInstall}/linux/xclient-static.sh ${sbInstall}/linux/xclient.sh`
14. Optionally configure Steam or your other launcher to launch `${sbInstall}/xsb-linux/xclient.sh`.

### Windows 10 or 11

To build and install xStarbound on Windows 10 or 11:

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/whatsnew/) and [CMake](https://cmake.org/download/). For Visual Studio, make sure to install C++ support (and optionally the game dev stuff) when the VS installer asks you what to install. For CMake, make sure you download and use the `.msi` installer for 64-bit Windows.
2. Install [Git](https://git-scm.com/download/win); the build script *requires* Git. Make sure to tick «Add to your PATH» in the installer.
3. Open up Git Bash and run `git clone https://github.com/xStarbound/xStarbound.git` *or* download the latest xStarbound source ZIP archive and extract it somewhere.
4. Go into `scripts\windows\` and double-click `build.bat`. CMake and VCPKG will take care of the entire build process.
5. If the build succeeds, you will be prompted to select your Starbound install directory.
6. Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

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
4. If you're using Git, run `git clone https://github.com/xStarbound/xStarbound.git` in a terminal or command prompt, or use a graphical Git utility to download the repo.
5. In the xStarbound directory, run `cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/`. On some OSes, you may need to add the full path to your CMake executable to the beginning of the command. If necessary, add `-DCMAKE_C_COMPILER=<path to C++ compiler> -DCMAKE_CXX_COMPILER=<path to C++ compiler>`. Note that a CMake build preset already exists for modern macOS; consider using that if you have build issues.

### Packaging and Installing xSB Assets

The build system can automatically take care of packaging the additional xSB assets and install the resulting .pak file automatically. To enable asset packing, pass `-DPACKAGE_XSB_ASSETS=ON` to the CMake command, e.g. after the `--preset` argument. This will create a directory `xsb-assets` in the project's main dir, and package the resources in `assets/xSBassets` into a `packed.pak` file.

It is important to note that automatic packaging will _only_ work if the built binaries (specifically asset_packer[.exe]) run on the host system. If you are cross-compiling, e.g. for a different CPU architecture (building for ARM64 on a x64 OS) or a different OS (building with MinGW on Linux, or with WSL on Windows), this feature cannot be used. In this case, you have to manually package the assets with an asset_packer that runs locally, and copy the resulting file to `xsb-assets/packed.pak` _before installing the project_. Due to how CPack works, this is only feasible when packaging manually from the install dir.

## Discord

For support, suggestions or just plain old chit-chat, check out the [xStarbound Discord server](https://discord.gg/GJ5RTkyFCX).
