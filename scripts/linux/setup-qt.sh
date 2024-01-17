#!/bin/sh
# Special setup file for building the Qt GUI stuff only. Needed because these GUIs need to use the system libpng, not the one included with the source.

cd "`dirname \"$0\"`/../..";

# rm -rI build-qt/ dist/mod_uploader dist/json_tool;

mkdir -p dist;
cp scripts/linux/xsbinit.config dist/;

mkdir -p build-qt;
cd build-qt;

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH;
fi

LINUX_LIB_DIR=../lib/linux;

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DSTAR_USE_JEMALLOC=ON \
  -DCMAKE_INCLUDE_PATH=$LINUX_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH=$LINUX_LIB_DIR/ \
  -DQT_GUIS_ONLY=ON \
  -DPNG_LIBRARY=/usr/lib/libpng16.so.16 \
  -DSTAR_BUILD_QT_TOOLS=ON \
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON \
  -S ../source/ -B .;

# RelWithAsserts

if [ $# -ne 0 ]; then
  make -j$*;
else
  make;
fi

cp -r ../dist/* ../test/linux/; # Will replace existing xSB test executables.