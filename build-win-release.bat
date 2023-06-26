@echo off
call vcvars64.bat
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cd build-release
rmdir /S /Q output
mkdir output
copy cpp-dev-tools.exe output
%CMAKE_PREFIX_PATH%\bin\windeployqt.exe output -qmldir=..\qml
