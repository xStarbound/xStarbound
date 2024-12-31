#!/bin/sh
# Script needed to launch xClient properly. You should be able to double-click this script in your file manager to start xClient.

sbPath="$(realpath -zL "$0" | xargs -0 dirname)/../"; export sbPath;
workingDir="$(pwd)";
cd "$(dirname "$0")";

# Use Wayland if it's what's running.
if [ -n "$WAYLAND_DISPLAY" ]; then 
  export SDL_VIDEODRIVER="wayland";
fi

if [ -f "${workingDir}/xsbinit.config" ]; then
  # Check for an `xsbinit.config` file in the working directory; if it exists, use that.
  LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient -bootconfig "${workingDir}/xsbinit.config" "$@";
else
  LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient "$@"; #"$0";
fi

SDL_VIDEODRIVER=; export SDL_VIDEODRIVER;
sbPath=; export sbPath;
