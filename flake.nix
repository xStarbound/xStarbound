{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.05";
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
      packages = self.packages.${system};
    in
    {

      packages.${system} = {

        fetchFromSteamWorkshop = pkgs.callPackage ./nix/fetchFromSteamWorkshop { };
        fetchStarboundMod = pkgs.callPackage ./nix/fetchStarboundMod.nix {
          inherit (packages) fetchFromSteamWorkshop;
        };

        xstarbound-raw = pkgs.callPackage ./nix/xstarbound-raw.nix { };
        xstarbound-app = pkgs.callPackage ./nix/xstarbound-app.nix {
          inherit (packages) xstarbound-raw;
        };
        xstarbound = pkgs.callPackage ./nix/xstarbound.nix {
          inherit (packages) xstarbound-raw;
        };
        default = packages.xstarbound;
      };

      legacyPackages.${system} = {
        mods = pkgs.callPackage ./nix/mods.nix {
          inherit (packages) fetchStarboundMod;
        };
      };

      apps.${system}.default = {
        type = "app";
        program = pkgs.lib.getExe packages.xstarbound-app;
      };

    };
}

