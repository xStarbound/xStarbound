#!/bin/sh
# Script needed to launch xSB-2 xClient properly. You should be able to double-click this script in your file manager to start xClient.
cd "`dirname \"$0\"`";
sbPath=$(realpath $(dirname "$0")); export sbPath;
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./xclient; #"$0";
sbPath=; export sbPath;
