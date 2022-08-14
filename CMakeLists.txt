cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS Quick Gui REQUIRED)
qt_add_executable(cpp-dev-tools src/main.cpp)
target_link_libraries(cpp-dev-tools PRIVATE Qt6::Quick Qt6::Gui)
qt_add_qml_module(cpp-dev-tools URI cdt VERSION 1.0 QML_FILES qml/main.qml)
