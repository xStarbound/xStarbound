#!/bin/sh
# Script needed to launch xSB-2 xClient properly. You should be able to double-click this script in your file manager to start xClient.
sbPath="$(realpath -zL $0 | xargs -0 dirname)/../"; export sbPath;
cd "`dirname \"$0\"`";
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient; #"$0";
sbPath=; export sbPath;
