#!/bin/sh
# This script is used for building xSB-2 in a Steam Scout Runtime container.

cd "`dirname \"$0\"`/../..";

#rm -rI build-runtime/ dist/;

mkdir -p dist;
cp scripts/linux/xsbinit.config dist/;

mkdir -p build-runtime;
cd build-runtime;

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH;
fi

LINUX_LIB_DIR=../lib/linux;

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-9 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-9 \
  -DCMAKE_INCLUDE_PATH=$LINUX_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH=$LINUX_LIB_DIR/ \
  -DSTAR_USE_JEMALLOC=ON \
  -DSTAR_USE_MIMALLOC=OFF \
  -DSTAR_BUILD_QT_TOOLS=OFF \
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON \
  -S ../source/ -B .;

# RelWithAsserts

if [ $# -ne 0 ]; then
  make -j$*;
else
  make;
fi