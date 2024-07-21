#!/bin/bash
# Script needed to launch xServer properly.

cd "`dirname \"$0\"`";
./xserver "$@";
exit $?;
