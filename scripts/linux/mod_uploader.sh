#!/bin/sh
# Script needed to launch the mod_uploader. You should be able to double-click this script and launch it from your file manager.
cd "`dirname \"$0\"`";
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./" ./mod_uploader "$@"; #"$0";
