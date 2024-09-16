{ steamSupport ? false
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
let fs = lib.fileset; in

stdenv.mkDerivation {
  pname = "xstarbound-raw";
  version = "v3.1.3r1";
  src = fs.toSource rec {
    root = ../.;
    fileset = fs.difference root (fs.unions [
      ### FILES TO EXCLUDE ###

      # NB: Technically, this list of path exclusions could be more aggressive,
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
    ]);
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

  installPhase = ''
    cmake --install . --prefix $out
    runHook postInstall
  '';

  postInstall = ''
    mkdir -p "$out/bin"
    ln -s "$out/linux/xclient" "$out/bin/xclient"
  '';

  meta.mainProgram = "xclient";
}
