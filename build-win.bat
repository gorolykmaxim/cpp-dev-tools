@echo off
call vcvars64.bat
if not exist build\ (
  mkdir build
  cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)
cmake --build build
