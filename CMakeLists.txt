cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS Quick Gui Concurrent REQUIRED)
qt_add_executable(cpp-dev-tools
  src/main.cpp
  src/common.hpp
  src/process/process.hpp
  src/process/process.cpp
  src/threads.hpp
  src/threads.cpp
  src/process/config.hpp
  src/process/config.cpp
  src/application.hpp
  src/application.cpp)
target_include_directories(cpp-dev-tools PRIVATE src)
target_link_libraries(cpp-dev-tools PRIVATE Qt6::Quick Qt6::Gui Qt6::Concurrent)
qt_add_qml_module(cpp-dev-tools URI cdt VERSION 1.0 QML_FILES qml/main.qml)
