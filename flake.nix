{
  description = "Fork of OpenStarbound and successor to xSB-2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
  };

  outputs =
    { self, ... }@inputs:
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

        # meta package for garnix which can't support legacyPackages; do not use!
        _allMods = pkgs.linkFarmFromDrvs "xstarbound-allMods" self.legacyPackages.${system}.mods;
      };

      nixosModules = {
        xstarbound = import ./nix/module.nix self;
        default = self.nixosModules.xstarbound;
      };

      legacyPackages.${system} = {
        fetchFromSteamWorkshop = pkgs.callPackage ./nix/fetchFromSteamWorkshop { };
        fetchStarboundMod = pkgs.callPackage ./nix/fetchStarboundMod.nix {
          inherit (self.legacyPackages.${system}) fetchFromSteamWorkshop;
        };
        dirwrap = pkgs.callPackage ./nix/dirwrap.nix { };
        mods = pkgs.callPackage ./nix/mods.nix {
          inherit (self.legacyPackages.${system}) fetchStarboundMod dirwrap;
        };
      };

      formatter.${system} = pkgs.nixfmt-rfc-style;

    };
}
