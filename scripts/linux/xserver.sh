#!/bin/bash
# Script needed to launch xServer properly.

sbPath="$(realpath -zL "$0" | xargs -0 dirname)/../"; export sbPath;
workingDir="$(pwd)";
cd "$(dirname \"$0\")";

if [ -f "${workingDir}/xsbinit.config" ]; then
  # Check for an `xsbinit.config` file in the working directory; if it exists, use that.
  ./xserver -bootconfig "${workingDir}/xsbinit.config" "$@";
else
  ./xserver "$@";
fi
exit $?;
