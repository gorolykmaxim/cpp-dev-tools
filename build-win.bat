@echo off
call vcvars64.bat
if not exist build\ (
  mkdir build
  cmake -B build -G Ninja
)
cmake --build build
