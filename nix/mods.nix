{ fetchurl
, fetchzip
, lib
, fetchStarboundMod
, dirwrap
,
}:
{

  frackin-universe =
    let
      release = "6.4.5";
    in
    dirwrap (fetchurl {
      url = "https://github.com/sayterdarkwynd/FrackinUniverse/releases/download/${release}/FrackinUniverse.pak";
      hash = "sha256-L2/pVQIckOFr/EGkXtYCsUe5SPFrhv+JemGpUB36T8s=";
    });

  enterable-fore-block = fetchStarboundMod {
    workshopId = "3025026792";
    hash = "sha256-gybKmQf4Pe52xTfFxXhahTmxYa0ry444FDoz/h/ZP94=";
  };

  fezzed-tech =
    let
      release = "v1.3.1";
    in
    dirwrap (fetchurl {
      url = "https://github.com/FezzedOne/FezzedTech/releases/download/${release}/FezzedTech.pak";
      hash = "sha256-PM+kubZ5bM0a5pSY7BVepCLb/JrLV6IH3GUDxNHCBBE=";
    });

  time-control =
    let
      release = "1.0";
    in
    dirwrap (fetchurl {
      url = "https://github.com/bongus-jive/time-control-command/releases/download/v${release}/TimeControl-${release}.pak";
      hash = "sha256-f0eFjglLlgEXa37yuJ4ooJcgThCKCdsxU5NlzJMUE0w=";
    });

  xwedit =
    let
      release = "v1.4.4.8/2.0.0";
    in
    dirwrap (fetchurl {
      url = "https://github.com/FezzedOne/xWEdit/releases/download/${release}/xWEdit.pak";
      hash = "sha256-3ENetp8mFpbo7FuY4A9dpiJ0/Q4ieH2r1CeMJAjPCYo=";
    });

  tech-loadout-binds = fetchStarboundMod {
    workshopId = "2920684844";
    hash = "sha256-IQIzVKYVoiKCzRGGnYuQJgN3o4/R/Qt4rORVJC8ujQ4=";
  };

  scanner-shows-printability = fetchStarboundMod {
    url = "https://steamcommunity.com/sharedfiles/filedetails/?id=3145469034";
    hash = "sha256-NtfKcw7AFDDUsEmXurRB3y0emiUiOcsOHDrgLb/lcDA=";
  };

}
