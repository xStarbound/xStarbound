#!/bin/sh
# Script needed to launch xClient properly. You should be able to double-click this script in your file manager to start xClient.
sbPath="$(realpath -zL $0 | xargs -0 dirname)/../"; export sbPath;
cd "`dirname \"$0\"`";

# Automatically switch to wayland support if it seems like it's running.
if [ -n "$WAYLAND_DISPLAY" ]; then 
  export SDL_VIDEODRIVER="wayland";
fi

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient "$@"; #"$0";

SDL_VIDEODRIVER=; export SDL_VIDEODRIVER;
sbPath=; export sbPath;
