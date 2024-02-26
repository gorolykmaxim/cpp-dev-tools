#!/usr/bin/env zsh

set -e
cmake -B build-release -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64' -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cd build-release
rm -f cpp-dev-tools.dmg
${CMAKE_PREFIX_PATH}/../../bin/macdeployqt cpp-dev-tools.app -qmldir=../qml -dmg
open .
