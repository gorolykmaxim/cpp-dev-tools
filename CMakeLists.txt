cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS
  Quick QuickControls2 Gui Concurrent Sql REQUIRED)
qt_add_executable(cpp-dev-tools
  src/main.cpp
  src/Lib.hpp
  src/PerfTimer.hpp
  src/PerfTimer.cpp
  src/Threads.hpp
  src/Threads.cpp
  src/UI.hpp
  src/UI.cpp
  src/Process.hpp
  src/Process.cpp
  src/Execution.hpp
  src/Execution.cpp
  src/Db.hpp
  src/Db.cpp
  src/UIData.hpp
  src/UIData.cpp
  src/ProcessData.hpp
  src/ProcessData.cpp
  src/AppData.hpp
  src/AppData.cpp
  src/JsonFileProcess.hpp
  src/JsonFileProcess.cpp
  src/SelectProject.hpp
  src/SelectProject.cpp
  src/ExecUserCmds.hpp
  src/ExecUserCmds.cpp
  src/SearchUserCmds.hpp
  src/SearchUserCmds.cpp
  src/CloseProject.hpp
  src/CloseProject.cpp
  src/ExecTasks.hpp
  src/ExecTasks.cpp
  src/ExecTask.hpp
  src/ExecTask.cpp
  src/ExecOSCmd.hpp
  src/ExecOSCmd.cpp
  src/InitSqlDb.hpp
  src/InitSqlDb.cpp
  src/ChooseFile.hpp
  src/ChooseFile.cpp
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
  Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Concurrent Qt6::Sql)
qt_add_qml_module(cpp-dev-tools URI cdt VERSION 1.0
  QML_FILES
      qml/MaterialDesignIcons.js
      qml/Common.js
      qml/main.qml
      qml/ButtonWidget.qml
      qml/IconWidget.qml
      qml/TextWidget.qml
      qml/TextFieldWidget.qml
      qml/PaneWidget.qml
      qml/ReadOnlyTextAreaWidget.qml
      qml/StatusBarWidget.qml
      qml/MenuBarWidget.qml
      qml/NativeMenuBarWidget.qml
      qml/CrossPlatformMenuBarWidget.qml
      qml/TextListWidget.qml
      qml/AlertDialog.qml
      qml/SearchUserCmdsDialog.qml
      qml/BlankView.qml
      qml/SelectProjectView.qml
      qml/ChooseFileView.qml
      qml/ExecTasksView.qml
  RESOURCES
      fonts/MaterialIcons-Regular.ttf)
