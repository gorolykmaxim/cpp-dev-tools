cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS
  Quick QuickControls2 Gui Concurrent Sql Test REQUIRED)
qt_add_executable(cpp-dev-tools
  src/database.h
  src/database.cc
  src/application.h
  src/application.cc
  src/theme.h
  src/project_controller.h
  src/project_controller.cc
  src/choose_file_controller.h
  src/choose_file_controller.cc
  src/qvariant_list_model.h
  src/qvariant_list_model.cc
  src/perf_timer.h
  src/perf_timer.cc
  src/user_command_system.h
  src/user_command_system.cc
  src/search_user_command_controller.h
  src/search_user_command_controller.cc
  src/task_list_controller.h
  src/task_list_controller.cc
  src/project_system.h
  src/project_system.cc
  src/view_system.h
  src/view_system.cc
  src/task_system.h
  src/task_system.cc
  src/task_execution_history_controller.h
  src/task_execution_history_controller.cc
  src/text_area_controller.h
  src/text_area_controller.cc
  src/main.cc)
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
      qml/Common.js
      qml/NativeMenuBar.qml
      qml/CrossPlatformMenuBar.qml
      qml/MenuBar.qml
      qml/SearchUserCommandDialog.qml
      qml/SearchableTextList.qml
      qml/TaskList.qml
      qml/TaskExecutionHistory.qml
      qml/IconButton.qml
  RESOURCES
      fonts/MaterialIcons-Regular.ttf)

qt_add_executable(example-test test/main.cc test/example_test.h
  test/another_example_test.h)
target_include_directories(example-test PRIVATE test)
target_link_libraries(example-test PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent
  Qt6::Gui)
