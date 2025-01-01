{ steamSupport ? false
, bootconfig ? null
, symlinkJoin
, writeTextFile
, writeShellApplication
, writeText
, runCommand
, makeWrapper
, lib
, stdenv
, cmake
, ninja
, zlib
, libpng
, freetype
, libvorbis
, libopus
, SDL2
, glew
, xorg
, ...
}:
let
  fs = lib.fileset;

  base = stdenv.mkDerivation {
    pname = "xstarbound";
    # parse version # from CMakeLists.txt. This might be brittle and show a very wonky version
    # in the future, but it's better than someone forgetting to update it.
    version =
      let
        lines = builtins.split "\n" (builtins.readFile ../CMakeLists.txt);
        prefix = "set(XSB_VERSION ";
        suffix = ")";
        statement = lib.findFirst (line: !(builtins.isList line) && lib.hasPrefix prefix line) null lines;
      in
      lib.pipe statement [
        (lib.removePrefix prefix)
        (lib.removeSuffix suffix)
      ];

    src = fs.toSource rec {
      root = ../.;
      fileset = fs.difference root (
        fs.unions [
          ### FILES TO EXCLUDE ###

          # Technically, this list of path exclusions could be more aggressive,
          # since Nix doesn't require nearly all build files residing in this repo.
          # But this would require gutting out these paths in the project CMakeList,
          # which seems like a lot of work for questionable gain.

          # xStarbound helper files for "normal" OSes
          ../lib # windows stuff
          ../macos # Nix on darwin probably does not need this

          # vcpkg (package manager files not used by Nix)
          ../vcpkg.json
          ../vcpkg-configuration.json

          # git
          ../.gitattributes
          ../.github
          ../.gitignore
          ../.gitmodules

          # IDE
          ../.vscode

          # Nix
          ./.
          ../flake.nix
          ../flake.lock

          # Code project FILES
          ../README.md
        ]
      );
    };
    cmakeFlags = [
      "-DPACKAGE_XSB_ASSETS=ON"
      # TODO: Steam support has not been tested for this derivation
      "-DSTAR_ENABLE_STEAM_INTEGRATION=${if steamSupport then "ON" else "OFF"}"
    ];

    # NB: This code specifically passes libopus to the linker. At the time of writing (2024-09-09),
    # the reason for why we have to do this is unknown. All other libraries gets automatically passed by
    # existing in nativeBuildInputs, but not libopus. This hack makes the build logs very noisy and it's not
    # very elegant, so if any future readers know what the issue might be, please improve this.
    env.CXXFLAGS =
      let
        opusObjects = builtins.attrNames (builtins.readDir "${libopus}/lib");
      in
      "-Wl,-rpath ${lib.concatMapStringsSep " " (obj: "${libopus}/lib/${obj}") opusObjects}";

    nativeBuildInputs = [
      cmake
      ninja
      zlib
      libpng
      freetype
      libvorbis
      libopus
      SDL2
      glew
      xorg.libSM
      xorg.libXi
    ];

    postInstall = ''
      mkdir -p "$out/bin"
      ln -s "$out/linux/xclient" "$out/bin/xstarbound"
    '';

    meta.mainProgram = "xstarbound";
  };

in
writeShellApplication (lib.fix (self: {
  name = "xstarbound";
  runtimeInputs = [ base ];
  passthru = {
    inherit base bootconfig;
  };
  runtimeEnv = {
    inherit bootconfig;
  };
  text = (lib.optionalString (bootconfig == null) ''
    cfg_dir="$HOME/.config/xStarbound"
    bootconfig="$cfg_dir/xsbinit.config"

    mkdir -p "$cfg_dir"
    cp -n --no-preserve=mode '${./xsbinit.config}' "$bootconfig"
  '') + ''
    if [ -n "$WAYLAND_DISPLAY" ]; then 
      SDL_VIDEODRIVER="wayland"
    else
      SDL_VIDEODRIVER=""
    fi

    exec env SDL_VIDEODRIVER="$SDL_VIDEODRIVER" xstarbound \
      -bootconfig "$bootconfig"
  '';
}))
