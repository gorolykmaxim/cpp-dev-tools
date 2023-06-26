@echo off
call vcvars64.bat
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cd build-release
rmdir /S /Q cpp-dev-tools
mkdir cpp-dev-tools
copy cpp-dev-tools.exe cpp-dev-tools
%CMAKE_PREFIX_PATH%\bin\windeployqt.exe cpp-dev-tools -qmldir=..\qml
