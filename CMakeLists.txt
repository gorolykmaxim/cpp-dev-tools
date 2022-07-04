cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/boost EXCLUDE_FROM_ALL)
add_subdirectory(lib/cameron314_concurrentqueue)
add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/skypjack_entt)
add_subdirectory(lib/googletest EXCLUDE_FROM_ALL)

target_compile_options(boost_process INTERFACE -Wno-deprecated-declarations)

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
    src/processes.h
    src/processes.cpp
    src/execution.h
    src/execution.cpp)
if(WIN32)
  target_sources(cdt PRIVATE src/os-api-win.cpp)
else()
  target_sources(cdt PRIVATE src/os-api-unix.cpp)
endif()
target_include_directories(cdt PUBLIC src PUBLIC lib)
target_link_libraries(cdt
  Boost::process
  nlohmann_json
  cameron314_concurrentqueue
  skypjack_entt)

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
