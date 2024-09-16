{ lib
, stdenvNoCC
, steamcmd
, cacert
, writeText
, steam-run
,
}: { appId
   , workshopId
   , name ? "steamworkshop-${appId}-${workshopId}"
   , hash
   ,
   }: stdenvNoCC.mkDerivation {
  inherit name appId workshopId;
  builder = ./builder.sh;
  buildInputs =
    let
      # In true Nix fashion, we don't want the steam client to do its auto-update on startup and instead
      # pin Steam data ourselves, to strengthen reproducibility.
      # To disable this auto-update, we start steamcmd with the flag -inihibitbootstrap.
      # Disabling the bootstrap in turn means that steamcmd will miss several dynamically linked libraries, 
      # which we instead copy over ourselves.
      pinnedSteamcmd = steamcmd.overrideAttrs (prev: {
        installPhase = (prev.installPhase or "") + ''
          sed -i '$d' "$out/bin/steamcmd"
          echo '${lib.getExe steam-run} "$STEAMROOT/steamcmd.sh" -inhibitbootstrap "$@"' >> "$out/bin/steamcmd"
          cp -r ${../pinnedSteam}/* "$out/share/steamcmd"
        '';
      });
    in
    [ pinnedSteamcmd ];
  SSL_CERT_FILE = "${cacert}/etc/ssl/certs/ca-bundle.crt";

  outputHash = hash;
  outputHashAlgo = "sha256";
  outputHashMode = "recursive";
}
