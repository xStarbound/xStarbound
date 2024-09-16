{ fetchFromSteamWorkshop }:

{ url ? null
, workshopId ? null
, ...
}@args:
fetchFromSteamWorkshop ({
  appId = "211820";
  workshopId =
    if (url == null && workshopId != null) then # standard: set from URL
      workshopId
    else if (url != null && workshopId == null) then # capture workshopId from URL
      (builtins.head (builtins.match ".*?id=([0-9]+).*" url))
    else
      (builtins.abort "Exactly one of url OR workshopId must be set!");

} // (builtins.removeAttrs args [ "url" ]))
