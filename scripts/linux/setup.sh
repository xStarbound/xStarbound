#!/bin/sh

cd "`dirname \"$0\"`/../..";

rm -rI build/ dist/;

mkdir -p dist;
cp scripts/linux/sbinit.config dist/;

mkdir -p build;
cd build;

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH;
fi

LINUX_LIB_DIR=../lib/linux;

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DSTAR_USE_JEMALLOC=ON \
  -DCMAKE_INCLUDE_PATH=$LINUX_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH=$LINUX_LIB_DIR/ \
  -DSTAR_BUILD_QT_TOOLS=OFF \
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON \
  -S ../source/ -B .;

# RelWithAsserts

if [ $# -ne 0 ]; then
  make -j$*;
else
  make;
fi

cp -r ../dist/* ../test/linux/; # Will replace existing xSB test executables.