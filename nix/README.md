# xStarbound with Nix

xStarbound features first-class Nix support. 2 main methods of running `xStarbound` exists: through the `app` and through the `package`.

Note:
- This section assumes beforehand knowledge about Nix and flakes. If you're new to either, refer to the relevant documentation.
- xStarbound requires flake-enabled Nix, and to run the command(s) below, the experimental nix3 CLI is required.
- Nix usage still requires stock game assets!

## App

You can run the xStarbound app with the following command:

```
nix run github:xstarbound/xstarbound
```

This will run `xclient` with the following parameters, writing config to a `mktemp` JSON file:
```bash
gog_assets_dir="$HOME/GOG Games/Starbound/game/assets"
steam_assets_dir="$HOME/.local/share/Steam/steamapps/common/Starbound/assets"
storage_dir="$HOME/.local/share/xStarbound/storage"
```
In other words, it looks for game assets in the standard GOG/Steam directories and puts mutable user data in `~/.local/share`. For standard configurations,
this will "just work".

### But what if I have a nonstandard game assets directory? What if I want mods?

Read on further!

## Package

The xStarbound app works great for ad-hoc demo purposes, but has a few downsides:
- Game assets lookup rely on `$HOME`, which is impure 
- Game assets are mutable
- Configuration parameters are set in stone
- Doesn't properly "install" it to your system
- No proper mod support
- No orthogonality; everything needs to reside in your game install directory

Therefore, a Nix `package` is also provided. Here's how to install it with NixOS:

1. `inputs.xstarbound.url = "github:xstarbound/xstarbound";`
2. Add the `xstarbound` package to your config. Configuration is done through an `.override`. Example:
  ```nix
  # /etc/nixos/xstarbound.nix
  # ----------------------------------------------------------------------------
  { inputs, pkgs, lib, ... }: let  #  assume inputs is passed as a specialArg

    # some shortcuts
    xsb-pkgs = inputs.xstarbound.packages.${pkgs.system};
    inherit (inputs.xstarbound.legacyPackages.${pkgs.system}) mods;

    # replace this with your username.
    homeDir  = "/home/bob";

    # How to fetch the assets directory is left as an exercise for the reader.
    #   - A simple method would be to host a zipfile of it on your homeserver/cloud storage and then pkgs.fetchzip it.
    #   - You could also (probably?) use Git LFS.
    #   - In a pinch, requireFile is a reasonable escape hatch.
    # Either way, this drv should evaluate to an "assets" directory which contains the file "packed.pak".
    #     (i.e. /nix/store/<hash>-<name>/packed.pak)
    assetsDirectory = pkgs.someSourceFetcherProbably { };

    # Mods! With the power of Nix, 
    # you can seperate your modded/vanilla Starbound installs from each other! :-)
    # This encapsulation is especially useful for mods like Frackin' Universe which permanently changes mutable user state,
    # and crashes if you later remove the mod. (@fezzedone: Don't need to worry about that anymore with xStarbound though.)
    mods = [

      # use a pre-packaged mod from xStarbound
      mods.frackin-universe 

      # grab a starbound mod from the steam workshop using an url
      (xsb-pkgs.fetchStarboundMod {
        url = "https://steamcommunity.com/sharedfiles/filedetails/?id=XXXXXXXXX";
        hash = lib.fakeHash;
      })

      # grab a starbound mod by workshopId
      (xsb-pkgs.fetchStarboundMod {
        workshopId = "XXXXXXXXX";
        hash = lib.fakeHash;
      })
    ];
  # ----------------------------------------------------------------------------

    # the resulting drv 
    xstarbound' = xstarbound.override { 
      storageDirectory = "${homeDir}/.local/share/xStarbound/storage";
      inherit assetsDirectory mods;
    };

  in {
    environment.systemPackages = [ xstarbound' ];
  }
  ```
3. Run `xstarbound`!

## Other systems? Other OSes?

Not explicitly tested, but should probably be easy to patch in. PRs welcome!

