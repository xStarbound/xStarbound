self:
{ config, pkgs, lib, ... }:
let
  cfg = config.programs.xstarbound;
  settingsFormat = pkgs.formats.json { };
in
{
  options.programs.xstarbound = {

    enable = lib.mkEnableOption "xstarbound";
    package = lib.mkPackageOption self.packages.${pkgs.system} "xstarbound" { };
    bootconfig.settings = lib.mkOption {
      default = { };
      type = settingsFormat.type;
    };
    finalPackage = lib.mkOption {
      readOnly = true;
      default = cfg.package.override {
        bootconfig = settingsFormat.generate "xsbinit.config" cfg.bootconfig.settings;
      };
    };
  };

  config = lib.mkIf cfg.enable {
    environment.systemPackages = [ cfg.finalPackage ];
    programs.xstarbound.bootconfig.settings.assetDirectories = [
      "../xsb-assets/"
    ];
  };
}
