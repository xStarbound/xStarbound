#!/bin/sh
# Script needed to launch xClient properly. You should be able to double-click this script in your file manager to start xClient.
sbPath="$(realpath -zL $0 | xargs -0 dirname)/../"; export sbPath;
cd "`dirname \"$0\"`";
if [[ -f /dev/dsp ]]; then
  LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient "$@"; #"$0";
else
  LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" padsp ./xclient "$@"; #"$0";
fi
sbPath=; export sbPath;
