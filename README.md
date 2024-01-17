# xSB-2

This is a fork of [OpenStarbound](https://github.com/OpenStarbound/OpenStarbound). Contributions are welcomed. You **must** own a copy of Starbound to use xSB-2. Base game assets are not provided for obvious reasons.

Compiled builds for Linux and Windows should be available in the usual place on this repository.

## Building

This repository is already set up for easy building. Follow the appropriate instructions for your OS if listed; if your OS *isn't* listed, adjustments generally shouldn't be too complex. Note that building with Clang/LLVM is *not* properly supported, and will likely never be — expect Clang builds to be a buggy mess.

### Linux

To build on any reasonably up-to-date Linux distro:

1. Make sure you have GCC installed; it should come preinstalled on most distros. If not, install your distribution's "base development" package.
2. Install CMake and optionally Git (if not already installed). Your distribution's packages should be called `cmake` and `git`, respectively.
3. Download the latest source tarball (or clone the repo) and extract.
4. `cd xSB-2/`
5. `git submodule init` or download the latest [Opus source tarball](https://github.com/xiph/opus/releases) and extract into `$src/source/extern/opus/`.
6. `scripts/linux/setup.sh 3` (increase that `3` to `4` or more if you've got a beefy system).
7. Executables, required `.so` libaries and the required `sbinit.config` should appear in `$src/dist` if built successfully.
8. `mkdir -p ${sbInstall}/xsb-linux; cp dist/* ${sbInstall}/xsb-linux/`
9. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
10. Optionally configure Steam or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `${sbInstall}/xsb-linux/xclient`.

> **Note:** Gentoo users will need to recompile `libstdc++` with `_GLIBCXX_USE_CXX11_ABI` enabled (`-D_GLIBCXX_USE_CXX11_ABI=1`) to avoid linker errors.

### SteamOS

To build on SteamOS:

1. Run the following commands:

    ```sh
    sudo steamos-readonly disable
    sudo pacman -S base-devel glibc linux-api-headers cmake
    sudo steamos-readonly enable
    ```

2. Follow the Linux instructions starting at step 3.

You will need to re-run the commands in step 1 every time you update SteamOS (and want to rebuild xSB-2).

### Windows

To build and install on Windows 10 or 11:

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/whatsnew/) and [CMake](https://cmake.org/download/). For Visual Studio, make sure to install C++ support (and optionally the game dev stuff) when the VS installer asks you what to install. For CMake, make sure you download and use the `.msi` installer for 64-bit Windows.
2. Download the latest source ZIP (or install [Git](https://git-scm.com/download/win) and clone the repo) and extract.
3. download the latest [Opus source ZIP](https://github.com/xiph/opus/releases), extract it, and put the `opus\` folder in `source\extern\` (or install Git, open up Git Bash, `cd` into  `xSB-2\` and run `git submodule init`).
4. Go into `scripts\windows\` and double-click `setup64.bat`.
5. Wait for that batch file to finish, go up two folders, open up `build\` and double-click `ALL_BUILD.vcxproj`.
6. Select **Build → Build Solution** in Visual Studio.
7. Executables, required `.dll` libraries and the required `sbinit.config` should appear in a new `xSB-2\dist\` folder if built successfully.
8. Make a new `xsb-win64\` folder in your Starbound install folder, and copy or move the `.exe`s and `.dll`s to it.
9. Make a new `xab-assets\` folder in your Starbound install folder, and copy the `assets\xSBassets` folder into that folder.
10. Optionally configure Steam, GoG or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `xsb-win64\xclient.exe`.

Building on earlier versions of Windows is not recommended, although it *should* still be possible to build xSB-2 on Windows 7, 8 or 8.1 if you can get VS 2022 installed.

### macOS

There is currently no working macOS toolchain set up. If you want to build on macOS anyway, you should try installing and using GCC. The compiler paths are configured in `scripts/osx/setup.command` (or the `setup.sh` script in the same directory, if you don't like `.command` scripts). You'll also need to make some changes to `source/CMakeLists.txt` so that GCC doesn't assume you're using Linux headers and the like.

### Cross-compilation from Linux to Windows

To cross-compile from Linux to Windows:

1. Install CMake, WINE, MinGW-w64 and optionally Git (if not already preinstalled).
  - **Arch-/Debian-based distros (`apt`/`pacman`):** Install `cmake`, `wine`, `mingw-w64` and `git`.
  - **Fedora-based distros (`yum`):** Install `cmake`, `wine`, `mingw64-\*` and `git`.
2. Install or build the MinGW versions of Freetype (using `--without-harfbuzz`), ZLib, GLEW and SDL2.
  - On the AUR, these are `mingw-w64-freetype2-bootstrap`, `mingw-w64-zlib`, `mingw-w64-glew` and `mingw-w64-zlib`, respectively.
  - For Arch users (*not* derivatives), there is a [binary repo](https://martchus.no-ip.biz/repo/arch/ownstuff) for these libraries, but you should still install `mingw-w64-freetype2-bootstrap` from the AUR.
3. Download the latest source tarball (or clone the repo) and extract.
4. `cd xSB-2/`
5. `scripts/mingw/setup.sh 3` (increase that `3` to `4` or more if you've got a beefy system).
6. Executables, required `.dll` libaries and the required `sbinit.config` should appear in `$src/dist-windows` if built successfully. Note that the Discord library is differently named due to an idiosyncrasy with the linker; do not rename it back.
7. `mkdir -p ${sbInstall}/xsb-win64; cp dist-windows/* ${sbInstall}/xsb-win64/`
8. `mkdir -p ${sbInstall}/xsb-assets; cp assets/xSBassets ${sbInstall}/xsb-assets/`
9. Optionally configure Steam or [MultiBound2](https://github.com/zetaPRIME/MultiBound2) to launch `${sbInstall}/xsb-win64/xclient.exe` through WINE/Proton (or on your Windows install).

> **Note:** Gentoo users will need to compile MinGW with `_GLIBCXX_USE_CXX11_ABI` enabled (`-D_GLIBCXX_USE_CXX11_ABI=1`) to avoid linker errors.

## Changes

- You can now make `.patch` files that are just merged in, early-beta-style (Kae). That's why the patch files in `assets/xSBassets` are unusually simple.
- Almost all Lua callbacks from the original xSB (by FezzedOne), `input` callbacks (by Kae), plus some extra `player` callbacks for compatibility with OpenStarbound mods and some StarExtensions mods. The `setSpecies` and `setIdentity` callbacks will not let you switch to a nonexistent species. Documentation has yet to be updated.
- Various crash fixes (Kae and FezzedOne).
- Character swapping (rewrite by Kae from StarExtensions): `/swap <name>` (case-insensitive substring matching) and `/swapuuid <uuid>` (requires a UUID; use the one in the player file name).
- Custom user input support with a keybindings menu (rewrite by Kae from StarExtensions).
- Positional voice chat that works on completely vanilla servers; is compatible with StarExtensions. This uses Opus for crisp, HD audios. Rewrite by Kae from StarExtensions.
  - Both menus are made available in the options menu rather than as a chat command.
- Multiple font support (switch fonts inline with `^font=name;`, `.ttf` assets are auto-detected). Added by Kae, fixed by FezzedOne.
- World lightmap generation has been moved off the main thread (Kae).
- Experimental changes to the storage of directives in memory to greatly reduce their impact on frametimes (Kae).
  - Works well when extremely long directives are used for "vanilla multiplayer-compatible" creations, like [generated](https://silverfeelin.github.io/Starbound-NgOutfitGenerator/) [clothing](https://github.com/FezzedOne/FezzedOne-Drawable-Generator).
- Client-side tile placement prediction (rewrite by Kae from StarExtensions).
  - You can also resize the placement area of tiles on the fly.
- Support for placing foreground tiles with a custom collision type (rewrite by Kae from StarExtensions; requires an OpenSB or xSB-2 server) and, via `world.placeMaterial()`, placing tiles not connected to existing ones (FezzedOne; requires an xSB-2 server). Tile placement with this feature is not network-compatible with servers that support the similar feature present in StarExtensions, although already-placed tiles work just fine. A [fork of WEdit](https://github.com/FezzedOne/xWEdit) with support for these features is available.
  - Additionally, objects can be placed under non-solid foreground tiles (Kae).
- Some minor polish to UI (FezzedOne and Kae).

## Discord

For support, suggestions or just plain old chit-chat, check out the [xSB-2 Discord server](https://discord.gg/GJ5RTkyFCX).
