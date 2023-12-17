#!/bin/sh -e

cd macos_binaries

cp ../scripts/osx/xsbinit.config .

./core_tests
./game_tests

