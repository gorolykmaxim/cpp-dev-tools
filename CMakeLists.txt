cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS Quick QuickControls2 Gui Concurrent REQUIRED)
qt_add_executable(cpp-dev-tools
  src/main.cpp
  src/Lib.hpp
  src/PerfTimer.hpp
  src/PerfTimer.cpp
  src/UserConfig.hpp
  src/UserConfig.cpp
  src/Threads.hpp
  src/Threads.cpp
  src/UI.hpp
  src/UI.cpp
  src/UIData.hpp
  src/UIData.cpp
  src/Application.hpp
  src/Application.cpp
  src/ProcessRuntime.hpp
  src/ProcessRuntime.cpp
  src/JsonFileProcess.hpp
  src/JsonFileProcess.cpp
  src/OpenProject.hpp
  src/OpenProject.cpp
  src/Initialize.hpp
  src/Initialize.cpp)
if(NOT MSVC)
  # TODO: figure out how to enable all warnings in MSVC without triggering
  # them in Qt sources
  target_compile_options(cpp-dev-tools PRIVATE
    -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments)
endif()
target_include_directories(cpp-dev-tools PRIVATE src)
target_link_libraries(cpp-dev-tools PRIVATE
  Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Concurrent)
qt_add_qml_module(cpp-dev-tools URI cdt VERSION 1.0
  QML_FILES
      qml/MaterialDesignIcons.js
      qml/main.qml
      qml/ButtonWidget.qml
      qml/IconWidget.qml
      qml/TextWidget.qml
      qml/TextFieldWidget.qml
      qml/PaneWidget.qml
      qml/ReadOnlyTextAreaWidget.qml
      qml/StatusBarWidget.qml
      qml/ContextMenuWidget.qml
      qml/ContextMenuItemWidget.qml
      qml/AlertDialog.qml
      qml/BlankView.qml
      qml/OpenProjectView.qml
  RESOURCES
      fonts/MaterialIcons-Regular.ttf)
