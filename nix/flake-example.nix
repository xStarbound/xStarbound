{
  # flake.nix
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.xstarbound.url = "github:xstarbound/xstarbound";

  outputs = { self, ... }@inputs: {
    nixosConfigurations."mycomputer" = inputs.nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      specialArgs = { inherit inputs; };
      modules = [
        # ...
        ({ inputs, pkgs, lib, ... }:
          let
            xsbPkgs = inputs.xstarbound.packages.${pkgs.system} // {
              inherit (inputs.xstarbound.legacyPackages.${pkgs.system}) mods;
            };
          in
          {
            environment.systemPackages = [
              (
                xsbPkgs.default.override {
                  # refer to ./xsbinit.config for all possible options.
                  # 'assetDirectories' here refer to vanilla, xSB and mod assets.
                  xsbinitOverlays = [

                    # set a custom savedata location
                    (final: prev: {
                      storageDirectory = "/mnt/savegames/xStarbound";
                    })

                    # Standard GOG and Steam game install paths are already in this list,
                    # but you can also add custom ones yourself.
                    # For the adventurous, you could try fetching (legally acquired) game assets
                    # from your home server etc with a fixed-output derivation, `pkgs.fetchurl` or otherwise. 
                    (final: prev: {
                      assetDirectories = (prev.assetDirectories or [ ]) ++ [
                        (pkgs.fetchurl { })
                      ];
                    })

                    # import an overlay from another file
                    (import ./assets.nix { })

                    # different way of adding mods
                    (final: prev: {
                      assetDirectories = (prev.assetDirectories or [ ]) ++ [

                        # use a pre-packaged mod from xStarbound
                        xsbPkgs.mods.frackin-universe

                        # grab a starbound mod from the steam workshop using an url
                        (xsbPkgs.fetchStarboundMod {
                          url = "https://steamcommunity.com/sharedfiles/filedetails/?id=XXXXXXXXX";
                          hash = lib.fakeHash;
                        })

                        # grab a starbound mod by workshopId
                        (xsbPkgs.fetchStarboundMod {
                          workshopId = "XXXXXXXXX";
                          hash = lib.fakeHash;
                        })

                        # If you're using a fixed-output derivation which yields a file and not a directory, 
                        # you **MUST** turn it into a directory, either by a `postFetch` or a wrapper derivation.
                        # xStarbound provides such a wrapper function under the name `dirwrap`.
                        xsbPkgs.dirwrap
                        (
                          pkgs.fetchFromGitHub { }
                        )
                      ];
                    })

                  ];
                }
              )
            ];

          })
        # ...
      ];
    };
  };
}

