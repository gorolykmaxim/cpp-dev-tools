cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)

execute_process(COMMAND git --version RESULT_VARIABLE ret OUTPUT_QUIET ERROR_QUIET)
if (NOT ret EQUAL 0)
    message(FATAL_ERROR "You need to have 'git' installed. I need to apply patches to libraries :(")
else()
    execute_process(COMMAND git apply --check --reverse ../tiny-process-library.patch WORKING_DIRECTORY lib/tiny-process-library RESULT_VARIABLE ret OUTPUT_QUIET ERROR_QUIET)
    if (NOT ret EQUAL 0)
        execute_process(COMMAND git apply ../tiny-process-library.patch WORKING_DIRECTORY lib/tiny-process-library)
    endif()
endif()

add_subdirectory(lib/tiny-process-library)
add_subdirectory(lib/cameron314_concurrentqueue)
add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/skypjack_entt)
add_subdirectory(lib/googletest EXCLUDE_FROM_ALL)

add_library(cdt STATIC
    src/cdt.h
    src/cdt.cpp
    src/os-api.cpp
    src/common.h
    src/common.cpp
    src/config.h
    src/config.cpp
    src/gtest.h
    src/gtest.cpp
    src/user-command.h
    src/user-command.cpp
    src/output.h
    src/output.cpp
    src/process.h
    src/process.cpp
    src/execution.h
    src/execution.cpp)
target_include_directories(cdt
    PUBLIC src
    PUBLIC lib/tiny-process-library
    PUBLIC lib/nlohmann_json
    PUBLIC lib/cameron314_concurrentqueue
    PUBLIC lib/skypjack_entt)
target_link_libraries(cdt tiny-process-library nlohmann_json cameron314_concurrentqueue skypjack_entt)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} cdt)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(TEST_TARGET ${PROJECT_NAME}-test)
add_executable(${TEST_TARGET} EXCLUDE_FROM_ALL
  test/test-lib.h
  test/test-lib.cpp
  test/misc-test.cpp
  test/launch-test.cpp
  test/o-test.cpp
  test/t-test.cpp
  test/t-gtest-test.cpp
  test/tr-test.cpp
  test/g-test.cpp
  test/gt-test.cpp
  test/gtr-test.cpp
  test/gf-test.cpp
  test/s-test.cpp
  test/gs-test.cpp
  test/d-test.cpp
  test/gd-test.cpp
  test/exec-test.cpp)
target_include_directories(${TEST_TARGET} PUBLIC test)
target_link_libraries(${TEST_TARGET} cdt gtest gmock gtest_main)
target_compile_definitions(${TEST_TARGET} PRIVATE
    "TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test/data\"")
