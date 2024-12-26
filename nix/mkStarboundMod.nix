{ lib }:
{ srcDrv
, requiresConfig ? { }
, requiresFreshSave ? false
, requiresXSBVer ? null
, deps ? [ ]
, warnUseless ? false
, warnPartialFeatures ? [ ]
, otherWarning ? ""
, ...
}@args: srcDrv.overrideAttrs (prev: final: {
  passthru.xsbmod =
    let
      val = {
        inherit
          requiresConfig
          requiresFreshSave
          requiresXSBVer
          deps
          ;
      };
      # add on warning if safeScripts=false exists
      safeScriptsWarning = lib.optionalString
        (requiresConfig.safeScripts or false) ''
        The mod ${final.name} requires "safeScripts=false"! This will 
        automatically disable xStarbound Lua sandboxing for any 
        consuming game derivation. 
      '';
      uselessWarning = lib.optionalString warnUseless ''
        The mod ${final.name} is useless! It will do nothing when installed.
      '';
    in
    if (otherWarning + safeScriptsWarning) == "" then
      val
    else builtins.warn otherWarning val;
})
      
