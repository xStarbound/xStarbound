#!/bin/sh

cd "`dirname \"$0\"`/../..";

rm -rI build-mingw/ dist-windows/;

mkdir -p dist-windows;
cp scripts/mingw/xsbinit.config dist-windows/;
cp lib/mingw/*.dll dist-windows/;
# FezzedOne: Need to rename this DLL because of a linking idiosyncrasy.
mv dist-windows/discord_game_sdk.dll dist-windows/discord_game_sdk_stub.dll;

mkdir -p build-mingw;
cd build-mingw;

if [ -d /usr/lib/ccache ]; then
  export PATH=/usr/lib/ccache/:$PATH;
fi

MINGW_LIB_DIR=../lib/mingw;

cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=/usr/bin/x86_64-w64-mingw32-windres \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DSTAR_CROSS_COMPILE=ON \
  -DCMAKE_INCLUDE_PATH=$MINGW_LIB_DIR/include \
  -DCMAKE_LIBRARY_PATH="/usr/x86_64-w64-mingw32/lib;$MINGW_LIB_DIR/" \
  -DSTAR_USE_MIMALLOC=ON \
  -DSTAR_SYSTEM="windows" \
  -DSTAR_SYSTEM_FAMILY="windows" \
  -DSTAR_ENABLE_STEAM_INTEGRATION=ON \
  -DSTEAM_API_LIBRARY="${MINGW_LIB_DIR}/steam_api64.dll" \
  -DSTEAM_API_INCLUDE_DIR="${MINGW_LIB_DIR}/include/steam" \
  -DSTAR_ENABLE_DISCORD_INTEGRATION=ON \
  -DDISCORD_API_LIBRARY="${MINGW_LIB_DIR}/discord_game_sdk.dll" \
  -DDISCORD_API_INCLUDE_DIR="${MINGW_LIB_DIR}/include" \
  -DSTAR_BUILD_QT_TOOLS=OFF \
  -S ../source/ -B .;

if [ $# -ne 0 ]; then
  make -j$*;
else
  make;
fi