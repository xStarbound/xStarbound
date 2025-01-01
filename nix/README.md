# xStarbound with Nix

xStarbound features first-class [Nix](https://nixos.org/) support. Everything is mostly OOTB and painless. 

- Basic familiarity with [flakes](https://www.tweag.io/blog/2020-05-25-flakes/) and [overriding](https://nixos.org/guides/nix-pills/14-override-design-pattern)
is highly recommended. 

- Only flake usage is *officially* supported but you could probably make flakeless usage work with some elbow grease.

- Only `x86_64-linux` support currently. PRs welcome!

## Quickstart (`nix run`)

1. Make sure you have vanilla Starbound assets installed in one of the following places:

- GOG install dir + `/assets`
- Steam install dir + `/assets`
- `$HOME/.local/share/xStarbound/assets`
- `$HOME/.local/share/Starbound/assets`

2. Run xStarbound
```
$ nix run github:xstarbound/xstarbound
```

This runs a wrapping script which does 3 things:
1. Since the package wasn't called with a custom bootconfig when running `nix run`, it writes some default configuration to 
`$HOME/.config/xStarbound` which most importantly gives xStarbound some game asset paths to try (see above).
2. If xStarbound seems to be running under Wayland, it enables Wayland compatibility.
3. Run xStarbound with `-bootconfig` set to `$HOME/.config/xStarbound/xsbinit.config`.

## NixOS module

1. Add to flake inputs:
```nix
# flake.nix
inputs.xstarbound.url = "github:xstarbound/xstarbound";
```

2. Add xStarbound to your `specialArgs`:
```nix
# flake.nix
  # ...
  nixpkgs.lib.nixosSystem {
    # system = ...
    # modules = [ ... ];
    specialArgs = {
      inherit (inputs) xstarbound;
    };
  };
```

3. Write & import the module. Here's an example one:

```nix 
# module.nix
{ xstarbound, ... }: {

  imports = [ xstarbound.nixosModules.xstarbound ];

  programs.xstarbound = {
    enable = true;
    bootconfig.settings = {
      assetDirectories = [
        # include game install on a different drive
        "/mnt/games/Starbound/assets"
      ];
    };
  };
}
```

## Mods

Starbound modding is simple; you can add mods to your game install by 
just copying over any `.pak` files you want to any `assetDirectory`. This can be done "normally" as you would on any other system.

But better yet, Starbound modding being this simple lends itself very well to Nix, and it becomes trivial to statelessly and reproducibly add
mods to your game. Here's how:

```nix
{ xstarbound, pkgs, lib, ...}: { 
  programs.xstarbound.bootconfig.settings = let 
    inherit (xstarbound.legacyPackages.${pkgs.system})
      mods
      fetchStarboundMod
      dirwrap
    ;
  in { 
    assetDirectories = [
    
      # use a pre-packaged mod from xStarbound
      mods.frackin-universe

      # grab a starbound mod from the steam workshop using an url
      (fetchStarboundMod {
        url = "https://steamcommunity.com/sharedfiles/filedetails/?id=XXXXXXXXX";
        hash = lib.fakeHash;
      })

      # grab a starbound mod by workshopId
      (fetchStarboundMod {
        workshopId = "XXXXXXXXX";
        hash = lib.fakeHash;
      })

      # If you're using a fixed-output derivation which yields a file and not a directory,
      # you **MUST** turn it into a directory, either by a `postFetch` or a wrapper derivation.
      # xStarbound provides such a wrapper function under the name `dirwrap`.
      (dirwrap (
        pkgs.fetchFromGitHub { 
          # ...
        }
      ))

    ];
  };
}
```
      
### Where's the home-manager module?

[Intentionally left out, because home-manager sucks and a lot less people should use it.](https://ayats.org/blog/no-home-manager). 

xStarbound is in the category of programs which supports stateless `--config` invocations very well, and as such there is no reason to use 
dotfile management software that clutters your `$HOME` when there's no need to. 

**So what should I use instead?**
If you're on NixOS, you should use the provided `nixosModule`. If you use Nix on another OS, this is one trivial way to do it:

1. `mkdir my-nix-packages && cd my-nix-packages`
2. Create the following `flake.nix`: 
```nix
{

  description = "my Nix packages";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    xstarbound.url = "github:xstarbound/xstarbound";
  };

  outputs = { self, ... }@inputs: let
    system = "x86_64-linux";
    pkgs = inputs.nixpkgs.legacyPackages.${system};
  in { 
    packages.${system}.xstarbound = inputs.xstarbound.packages.${system}.override {
      # you could create the JSON like this...
      bootconfig = pkgs.writeText "xsbinit.config" ''
        {
          "assetDirectories": [ 
            ...
          ],
          ...
        }
      '';
      # ...or this.
      bootconfig = pkgs.writeText "xsbinit.config" (builtins.toJSON {
        assetDirectories = [
          ...
        ];
      });
    };
  };
}
```

3. Add a shell alias for xStarbound:

```bash
echo 'alias xstarbound="nix run path:$HOME/my-nix-packages#xstarbound"' >> ~/.bashrc
```

4. `$ xstarbound` and you're done.
