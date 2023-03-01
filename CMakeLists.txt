cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS
  Quick QuickControls2 Gui Concurrent Sql Test REQUIRED)
qt_add_executable(cpp-dev-tools
  src/Database.hpp
  src/Database.cpp
  src/Application.hpp
  src/Application.cpp
  src/Theme.hpp
  src/ProjectController.hpp
  src/ProjectController.cpp
  src/ChooseFileController.hpp
  src/ChooseFileController.cpp
  src/QVariantListModel.hpp
  src/QVariantListModel.cpp
  src/PerfTimer.hpp
  src/PerfTimer.cpp
  src/Project.hpp
  src/Project.cpp
  src/UserCommandController.hpp
  src/UserCommandController.cpp
  src/UserCommandListModel.hpp
  src/UserCommandListModel.cpp
  src/SearchUserCommandController.hpp
  src/SearchUserCommandController.cpp
  src/ChooseTaskController.hpp
  src/ChooseTaskController.cpp
  src/ProjectContext.hpp
  src/ProjectContext.cpp
  src/main.cpp)
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
      qml/main.qml
      qml/Button.qml
      qml/Icon.qml
      qml/ListSearch.qml
      qml/MaterialDesignIcons.js
      qml/Pane.qml
      qml/SelectProject.qml
      qml/Text.qml
      qml/TextField.qml
      qml/TextList.qml
      qml/ChooseFile.qml
      qml/AlertDialog.qml
      qml/ReadOnlyTextArea.qml
      qml/StatusBar.qml
      qml/UserCommands.qml
      qml/Common.js
      qml/NativeMenuBar.qml
      qml/CrossPlatformMenuBar.qml
      qml/MenuBar.qml
      qml/SearchUserCommandDialog.qml
      qml/SearchableTextList.qml
      qml/RunTask.qml
  RESOURCES
      fonts/MaterialIcons-Regular.ttf)

qt_add_executable(example-test test/ExampleTest.cpp)
target_include_directories(example-test PRIVATE test)
target_link_libraries(example-test PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent)
