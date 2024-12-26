{
  description = "Fork of OpenStarbound and successor to xSB-2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
  };

  outputs = { self, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = import inputs.nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
      packages = self.packages.${system};
    in
    {

      packages.${system} = {
        xstarbound = pkgs.callPackage ./nix/package.nix { };
        default = packages.xstarbound;

        fetchFromSteamWorkshop = pkgs.callPackage ./nix/fetchFromSteamWorkshop { };
        fetchStarboundMod = pkgs.callPackage ./nix/fetchStarboundMod.nix {
          inherit (packages) fetchFromSteamWorkshop;
        };

      };

      legacyPackages.${system} = {
        mods = pkgs.callPackage ./nix/mods.nix {
          inherit (packages) fetchStarboundMod;
        };
      };

    };
}

