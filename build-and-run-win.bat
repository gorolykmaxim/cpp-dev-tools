@echo off
call vcvars64.bat
if not exist build\ (
  mkdir build
  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
)
cmake --build build
start /b /wait ./build/cpp-dev-tools.exe
