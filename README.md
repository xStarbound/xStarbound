# xSB-2

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xSB-2. Base game assets are not provided for obvious reasons.

Compiled builds for Linux and Windows should be available in the usual place on this repository.

## Changes

- You can now make `.patch` files that are just merged in, early-beta-style (Kae). That's why the patch files in `assets/xSBassets` are unusually simple. In addition, a new `"find"` parameter is supported for `"remove"` and `"replace"` operations where the `"path"` is to a JSON array (FezzedOne) — if `"find"` is present, only the first array value exactly matching the value in `"find"` is removed or replaced by `"value"`, not the entire array.
- Almost all Lua callbacks from the original xSB (by FezzedOne), `input` callbacks (by Kae), plus some extra `player` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods. The `setSpecies` and `setIdentity` callbacks will not let you switch to a nonexistent species. Documentation has yet to be updated.
- Various crash fixes (FezzedOne and Kae).
- Character swapping (rewrite by Kae from StarExtensions): `/swap <name>` (case-insensitive substring matching) and `/swapuuid <uuid>` (requires a UUID; use the one in the player file name).
- Custom user input support with a keybindings menu (rewrite by Kae from StarExtensions).
- Positional voice chat that works on completely vanilla servers; is compatible with StarExtensions. This uses Opus for crisp, HD audios. Rewrite by Kae from StarExtensions.
  - The voice chat configuration dialogue is made available in the options menu rather than as a chat command.
- Multiple font support (switch fonts inline with `^font=name;`, `.ttf` assets are auto-detected). Added by Kae, fixed by FezzedOne. Additionally, escape codes and custom fonts wrap and propagate across wrapped lines properly in the chat box (FezzedOne).
- World lightmap generation has been moved off the main thread (Kae).
- Various changes to the storage of directives and images in memory to greatly reduce their impact on frametimes (Kae).
  - Works well when extremely long directives are used for "vanilla multiplayer-compatible" creations, like [generated](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) [clothing](https://github.com/FezzedOne/FezzedOne-Drawable-Generator).
- Client-side tile placement prediction (rewrite by Kae from StarExtensions).
  - You can also resize the placement area of tiles on the fly.
- Support for placing foreground tiles with a custom collision type (rewrite by Kae from StarExtensions; requires an OpenSB or xSB-2 server) and, via `world.placeMaterial()`, placing tiles not connected to existing ones (FezzedOne; requires an xSB-2 server). Tile placement with this feature is not network-compatible with servers that support the similar feature present in StarExtensions, although already-placed tiles work just fine. A [fork of WEdit](https://github.com/FezzedOne/xWEdit) with support for these features is available.
  - Additionally, objects can be placed under non-solid foreground tiles (Kae).
- Some polish to UI (FezzedOne and Kae).
- Terraria-like placement animations for objects, tiles and liquids (FezzedOne).

## Mod compatibility

Read this to see if xSB-2 is compatible with your mods.

**Has xSB-2 support:** The following mods have special functionality that requires or is supported by xSB-2.

- [Actionbar Group Scrolling](https://steamcommunity.com/sharedfiles/filedetails/?id=3051031813) — fully supported by xSB-2.
- [boner guy](https://steamcommunity.com/sharedfiles/filedetails/?id=2992238651) — fully supported by xSB-2.
- [Enterable Fore Block](https://steamcommunity.com/sharedfiles/filedetails/?id=3025026792) — fully supported by xSB-2.
- [FezzedTech](https://steamcommunity.com/sharedfiles/filedetails/?id=2962923060) ([GitHub](https://github.com/FezzedOne/FezzedTech)) — requires xSB-2 for full functionality, but also supports StarExtensions (with reduced functionality) and is compatible with vanilla Starbound.
- [Infinite Inventory](https://steamcommunity.com/sharedfiles/filedetails/?id=1944652893) — the keybind is supported by xSB-2.
- [More Action Bar Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2962464896) — fully supported by xSB-2.
- [Ruler](https://steamcommunity.com/sharedfiles/filedetails/?id=2451043851) — keybinds are supported by xSB-2.
- [Tech Loadout Binds](https://steamcommunity.com/sharedfiles/filedetails/?id=2920684844&searchtext=starextensions) — fully supported by xSB-2.
- [Scanner Shows Printability](https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034) — fully supported by xSB-2 as of v2.3.7.
- [xAdvancedChat](https://github.com/FezzedOne/xAdvancedChat) — requires xSB-2 v2.3.7+. Supports all features of and is fully network-compatible with "upstream" [StarCustomChat](https://github.com/KrashV/StarCustomChat).
- [xWEdit](https://github.com/FezzedOne/xWEdit) — this WEdit fork requires xSB-2 for full functionality, but is partially supported by OpenStarbound (no mid-air tile placement) and compatible with vanilla Starbound (with no extra functionality above WEdit).

**Compatible:** Any mod not listed in the "partially compatible" or "not compatible" category should be compatible. Major mods that have been tested to be compatible:

- [Arcana](https://steamcommunity.com/workshop/filedetails/?id=2359135864).
- [Avali (Triage) Race Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=729558042).
- [Elithian Races Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=850109963).
- [Frackin' Universe](https://steamcommunity.com/sharedfiles/filedetails/?id=729480149) ([GitHub](https://github.com/sayterdarkwynd/FrackinUniverse)).
- [Maple32](https://steamcommunity.com/sharedfiles/filedetails/?id=2568667104&searchtext=maple32).
- [Project Knightfall](https://steamcommunity.com/sharedfiles/filedetails/?id=2010883172).
- [Shellguard: Starbound Expansion Remastered](https://steamcommunity.com/sharedfiles/filedetails/?id=1563376005).
- [Stardust Core Lite](https://steamcommunity.com/sharedfiles/filedetails/?id=2512589532) — although compatibility *might* be lost in future versions.

> **Note:** xSB-2 does not and will not support StarExtensions' "body dynamics" and text-to-speech features, and currently doesn't support StarExtensions' species-specific head rotation parameters. Details:
>
> - Armour, clothing and race mods with included SE "body dynamics" support are compatible, but the "non-jiggle" sprites will be displayed.
> - Race *and* race-modifying mods with StarExtensions head rotation parameters, such as [Nekify](https://steamcommunity.com/sharedfiles/filedetails/?id=2875605913) may have visual sprite glitches — such as Neki ears being clipped off — while xSB-2's head rotation is enabled.
> - Mods intended to patch in "body dynamics" support or StarExtensions-specific head rotation parameters for other mods simply will not work at all.
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

- [Quickbar Mini](https://steamcommunity.com/sharedfiles/filedetails/?id=1088459034) — will log script errors and show an empty menu on xSB-2 v2.3.7+ and is deprecated by the author anyway; use [Stardust Core Lite](https://steamcommunity.com/sharedfiles/filedetails/?id=251258953) instead.
- [StarCustomChat](https://github.com/KrashV/StarCustomChat) — use the fully supported (xAdvancedChat)[https://github.com/FezzedOne/xAdvancedChat] fork instead.
- [Remote Module](https://steamcommunity.com/sharedfiles/filedetails/?id=2943917766) — won't work and is likely to log script errors.
- [StarExtensions](https://github.com/StarExtensions/StarExtensions) — won't load and may cause crashes!
- [Text to Speech Droids](https://steamcommunity.com/sharedfiles/filedetails/?id=2933125939) — won't do anything.
- [Quick Commands!](https://steamcommunity.com/sharedfiles/filedetails/?id=3145473452) — this mod explicitly checks for StarExtensions, so the keybinds do not work, although the added hidden chat commands do.
- [Zoom Keybinds](https://steamcommunity.com/sharedfiles/filedetails/?id=2916058850) — will log script errors (xSB-2 has differently named callbacks) and is redundant anyway because xSB-2 already fully supports this feature.
- Mods that patch in StarExtensions "body dynamics" support for other mods. These won't do anything.

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

1. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's "base development" package.
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

> **Note:** If you get a "`libpng` not found" linker error while building, change the `libpng` path in `$src/scripts/linux/setup-qt.sh` to wherever `libpng16.so.16` is installed on your system.

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
3. Open up Git Bash and run `git clone --recurse-submodules https://github.com/FezzedOne/xSB-2.git`, then go to step 7.
4. Download the latest xSB-2 source ZIP and extract it somewhere.
5. Download the latest [Opus source ZIP](https://github.com/xiph/opus/releases), extract it, and put the `opus\` folder in `source\extern\`
6. Go into `scripts\windows\` and double-click `setup64.bat`.
7. Wait for that batch file to finish, go up two folders, open up `build\` and double-click `ALL_BUILD.vcxproj`.
8. Select **Build → Build Solution** in Visual Studio.
9.  Executables, required `.dll` libraries and the required `sbinit.config` should appear in a new `xSB-2\dist\` folder if built successfully.
10. Make a new `xsb-win64\` folder in your Starbound install folder, and copy or move the `.exe`s and `.dll`s to it.
11. Make a new `xab-assets\` folder in your Starbound install folder, and copy the `assets\xSBassets` folder into that folder.
12. Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

Building on earlier versions of Windows is not recommended, although it *should* still be possible to build xSB-2 on Windows 7, 8 or 8.1 if you can get VS 2022 installed.

### macOS

There is currently no working macOS toolchain set up. If you want to build on macOS anyway, you should try installing and using GCC. The compiler paths are configured in `scripts/osx/setup.command` (or the `setup.sh` script in the same directory, if you don't like `.command` scripts). You'll also need to make some changes to `source/CMakeLists.txt` so that GCC doesn't assume you're using Linux headers and the like.

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
