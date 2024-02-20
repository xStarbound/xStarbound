#!/bin/sh

cd "$(dirname $0)/../.."

cd build
# Assumes the dev environment has GLEW installed via Brew and SDL2 installed from the `.dmg` on GitHub.
make -j3 && \
install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/SDL2.framework/Versions/A/SDL2 ../dist/xclient && \
install_name_tool -change /usr/local/opt/glew/lib/libGLEW.2.2.dylib @executable_path/libGLEW.2.2.dylib ../dist/xclient