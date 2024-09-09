{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.05";
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {

      packages.${system} = {
        steam = pkgs.callPackage ./nix/package.nix { steamSupport = true; };
        gog = pkgs.callPackage ./nix/package.nix { steamSupport = false; };
      };

      apps.${system} = {
        installGOG = {
          type = "app";
          program = pkgs.lib.getExe (
            pkgs.writeShellApplication
              {
                name = "xstarbound-install-gog";
                text =
                  let
                    pkg = self.packages.${system}.gog;
                  in
                  ''
                    if ! [ -f "gameinfo" ]; then
                      echo "gameinfo file not found in current dir. Are you really in the GOG base path?"
                      exit 1
                    fi

                    find ${pkg} -not -type d -printf '%P\n' \
                      | xargs -I {} install -Dm644 "${pkg}/{}" "game/{}"

                    chmod u+x,g+x game/linux/{xserver,xclient}


                    echo 'Installed xclient and xserver to ./game/linux successfully.'
                    echo 'Usage:'
                    echo '$ ./game/linux/xclient'
                  '';
              });
        };
      };
    };
}

