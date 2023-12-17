#!/bin/sh -e

cd linux_binaries

cp ../scripts/linux/xsbinit.config .

./core_tests
./game_tests
