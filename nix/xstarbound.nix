{ xstarbound-raw
, assetsDirectory ? null
, storageDirectory ? null
, extraJSON ? { }
, writeTextFile
, writeShellApplication
}:
let

  defaults = {

    assetDirectories = [
      assetsDirectory
      "../xsb-assets/"
    ];

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

  xsbconfig = writeTextFile {
    name = "xsbinit.config";
    text = builtins.toJSON (defaults // extraJSON);
  };
in
writeShellApplication {
  name = "xstarbound";
  runtimeInputs = [ xstarbound-raw ];
  derivationArgs.passthru = { inherit xstarbound-raw xsbconfig; };
  text = ''
    xclient \
      -bootconfig ${xsbconfig}
      "$@"
  '';
}
   

