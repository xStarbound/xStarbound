{ xstarbound-raw
, assetsDirectory ? null
, storageDirectory ? null
, mods ? [ ]
, extraAttrs ? { }
, writeTextFile
, writeShellApplication
, lib
, runCommandLocal
}:
let
  mods' = builtins.map
    (mod:
      if !(lib.pathIsDirectory mod) then
        (runCommandLocal "${mod.name}-dirwrapper" { } ''
          mkdir -p "$out"
          ln -s ${mod} "$out/${mod.name}"
        '')
      else mod)
    mods;

  defaults = {

    assetDirectories = [
      assetsDirectory
      "../xsb-assets/"
    ] ++ mods';

    inherit storageDirectory;

    assetsSettings = {
      pathIgnore = [ ];
      digestIgnore = [ ".*" ];
    };

    defaultConfiguration = {
      allowAdminCommandsFromAnyone = true;
      anonymousCOnnectionsAreAdmin = true;
    };
  };

  mergedConfig = defaults // extraAttrs;

  xsbconfig = writeTextFile {
    name = "xsbinit.config";
    text = builtins.toJSON mergedConfig;
  };
in
writeShellApplication {
  name = "xstarbound";
  runtimeInputs = [ xstarbound-raw ];
  derivationArgs.passthru = { inherit xstarbound-raw xsbconfig; };
  text = ''
    mkdir -p "${mergedConfig.storageDirectory}"
    xclient \
      -bootconfig ${xsbconfig}
      "$@"
  '';
}
   

