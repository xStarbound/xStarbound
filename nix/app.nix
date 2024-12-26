{
  xstarbound-raw,
  writeShellApplication,
}:
writeShellApplication {
  name = "xstarbound-${xstarbound-raw.version}";
  runtimeInputs = [ xstarbound-raw ];
  text = ''
    gog_assets_dir="$HOME/GOG Games/Starbound/game/assets"
    steam_assets_dir="$HOME/.local/share/Steam/steamapps/common/Starbound/assets"
    storage_dir="$HOME/.local/share/xStarbound/storage"

    mkdir -p "$storage_dir"
    tmp_cfg="$(mktemp -t xstarbound.XXXXXXXX)"

    cat << EOF > "$tmp_cfg"
      {
      "assetDirectories" : [
        "$gog_assets_dir",
        "$steam_assets_dir",
        "../xsb-assets/",
        "../mods/"
      ],

      "storageDirectory" : "$storage_dir",

      "assetsSettings" : {
        "pathIgnore" : [],
        "digestIgnore" : [
          ".*"
        ]
      },

      "defaultConfiguration" : {
        "allowAdminCommandsFromAnyone" : true,
        "anonymousConnectionsAreAdmin" : true
      }
    }
    EOF

    xclient \
      -bootconfig "$tmp_cfg"
      "$@"

    rm "$tmp_cfg"
  '';
}
