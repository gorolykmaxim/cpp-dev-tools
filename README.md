# cpp-dev-tools
A tool to automate command execution and much more.

## Prerequisistes for building
Environment variables you need to set:
- CMAKE_PREFIX_PATH

Dependencies you need to install manually:
- Qt 6.5.1 for MacOS

On windows you need to have a ninja binary as well as a vcvars64.bat on your PATH.

## Why?
I used to use CLion. And i liked it a lot. In fact i've used Jetbrains' stuff most of my career.
However, CLion's autocomplete became slower and slower over time up to the point where i've said that enough is enough and decided to switch to a more lightweight editors with more responsive autocomplete and typing experience.

There are things in CLion however, that i can't live without, such as run configurations and gtest integration (and maybe some other stuff i can't remember right now). The code editors, i'm moving to, either don't have those features at all, or do have them but not in a state that i personally find usefull.

The purpose of this tool is to fill-in those gaps, while not being tied to any particular code editor as a plugin.

## How to build for development
On Mac:
```
mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
On Windows:
```
./build-and-run-win.bat
```
