cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS
  Quick QuickControls2 Gui Concurrent Sql Test REQUIRED)
add_subdirectory(lib/skypjack_entt)
add_subdirectory(lib/googletest)
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
  src/text_list_model.h
  src/text_list_model.cc
  src/perf_timer.h
  src/perf_timer.cc
  src/user_command_system.h
  src/user_command_system.cc
  src/search_user_command_controller.h
  src/search_user_command_controller.cc
  src/task_list_model.h
  src/task_list_model.cc
  src/project_system.h
  src/project_system.cc
  src/view_system.h
  src/view_system.cc
  src/task_system.h
  src/task_system.cc
  src/text_area_controller.h
  src/text_area_controller.cc
  src/task_execution_list_model.h
  src/task_execution_list_model.cc
  src/task_execution_controller.h
  src/task_execution_controller.cc
  src/editor_system.h
  src/editor_system.cc
  src/settings_controller.h
  src/settings_controller.cc
  src/notification_system.h
  src/notification_system.cc
  src/notification_list_controller.h
  src/notification_list_controller.cc
  src/ui_icon.h
  src/io_task.h
  src/find_in_files_controller.h
  src/find_in_files_controller.cc
  src/git_system.h
  src/git_system.cc
  src/documentation_system.h
  src/documentation_system.cc
  src/sqlite_list_controller.h
  src/sqlite_list_controller.cc
  src/path.h
  src/path.cc
  src/sqlite_system.h
  src/sqlite_system.cc
  src/open_sqlite_file_controller.h
  src/open_sqlite_file_controller.cc
  src/sqlite_table_model.h
  src/sqlite_table_model.cc
  src/sqlite_query_editor_controller.h
  src/sqlite_query_editor_controller.cc
  src/syntax.h
  src/syntax.cc
  src/sqlite_table_controller.h
  src/sqlite_table_controller.cc
  src/os_command.h
  src/os_command.cc
  src/git_branch_list_controller.h
  src/git_branch_list_controller.cc
  src/placeholder.h
  src/placeholder.cc
  src/new_git_branch_controller.h
  src/new_git_branch_controller.cc
  src/promise.h
  src/git_commit_controller.h
  src/git_commit_controller.cc
  src/git_diff_model.h
  src/git_diff_model.cc
  src/git_log_model.h
  src/git_log_model.cc
  src/git_show_model.h
  src/git_show_model.cc
  src/git_file_diff_controller.h
  src/git_file_diff_controller.cc
  src/run_cmake_controller.h
  src/run_cmake_controller.cc
  src/test_execution_model.h
  src/test_execution_model.cc
  src/qtest_execution_model.h
  src/qtest_execution_model.cc
  src/main.cc)
if(NOT MSVC)
  # TODO: figure out how to enable all warnings in MSVC without triggering
  # them in Qt sources
  target_compile_options(cpp-dev-tools PRIVATE
    -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments)
endif()
target_include_directories(cpp-dev-tools PRIVATE src)
target_link_libraries(cpp-dev-tools PRIVATE
  Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Concurrent Qt6::Sql skypjack_entt)
set(RESOURCES fonts/MaterialIcons-Regular.ttf)
if(WIN32)
  list(APPEND RESOURCES win/cpp-dev-tools.ico)
endif()
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
      qml/StatusBar.qml
      qml/Common.js
      qml/NativeMenuBar.qml
      qml/CrossPlatformMenuBar.qml
      qml/MenuBar.qml
      qml/SearchUserCommandDialog.qml
      qml/SearchableTextList.qml
      qml/TaskList.qml
      qml/IconButton.qml
      qml/TaskExecutionList.qml
      qml/TaskExecution.qml
      qml/Settings.qml
      qml/NotificationList.qml
      qml/FolderList.qml
      qml/FindInFiles.qml
      qml/CheckBox.qml
      qml/SplitViewHandle.qml
      qml/Documentation.qml
      qml/SqliteList.qml
      qml/OpenSqliteFile.qml
      qml/StatusBarItem.qml
      qml/SqliteQueryEditor.qml
      qml/TableView.qml
      qml/SqliteFileSelectedChecker.qml
      qml/PlaceholderText.qml
      qml/SqliteTable.qml
      qml/GitBranchList.qml
      qml/NewGitBranch.qml
      qml/GitCommit.qml
      qml/BigTextArea.qml
      qml/TextAreaSearchBar.qml
      qml/SmallTextArea.qml
      qml/FileLinkLookup.qml
      qml/DynamicContextMenu.qml
      qml/TextAreaLine.qml
      qml/GitDiff.qml
      qml/TextSearchBar.qml
      qml/GitLog.qml
      qml/GitShow.qml
      qml/GitFileDiff.qml
      qml/RunCmake.qml
      qml/TestExecution.qml
      qml/QtestExecution.qml
      qml/SetTestFilter.qml
  RESOURCES
      ${RESOURCES}
  RESOURCE_PREFIX /)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(APPLE)
    target_sources(cpp-dev-tools PRIVATE mac/cpp-dev-tools.icns)
    set_target_properties(cpp-dev-tools PROPERTIES
      MACOSX_BUNDLE true
      MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/mac/Info.plist)
    set_source_files_properties(mac/cpp-dev-tools.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  else()
    set_target_properties(cpp-dev-tools PROPERTIES WIN32_EXECUTABLE true)
    target_sources(cpp-dev-tools PRIVATE ${PROJECT_SOURCE_DIR}/win/cpp-dev-tools.rc)
  endif()
endif()

qt_add_executable(example-test test/main.cc test/example_test.h
  test/another_example_test.h)
target_include_directories(example-test PRIVATE test)
target_link_libraries(example-test PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent
  Qt6::Gui)

add_executable(example-gtest test/gtest-example.cc)
target_link_libraries(example-gtest gtest gmock gtest_main)
