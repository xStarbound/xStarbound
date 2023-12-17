#!/bin/sh

cd "$(dirname $0)/../.."

mkdir -p dist
cp scripts/osx/xsbinit.config dist/

mkdir -p build
cd build

CC=clang CXX=clang++ /Applications/CMake.app/Contents/bin/cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=true \
  -DCMAKE_BUILD_TYPE=Release \
  -DSTAR_USE_JEMALLOC=ON \
  -DCMAKE_INCLUDE_PATH=../lib/osx/include \
  -DCMAKE_LIBRARY_PATH=../lib/osx/ \
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON \
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON \
  -S ../source -B .;
