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
add_subdirectory(lib/googletest EXCLUDE_FROM_ALL)

add_library(cdt STATIC include/cdt.h src/cdt.cpp)
target_include_directories(cdt PUBLIC include PUBLIC lib/tiny-process-library PUBLIC lib/nlohmann_json PUBLIC lib/cameron314_concurrentqueue)
target_link_libraries(cdt tiny-process-library nlohmann_json cameron314_concurrentqueue)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} cdt)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(TEST_TARGET ${PROJECT_NAME}-test)
add_executable(${TEST_TARGET} EXCLUDE_FROM_ALL test/test.cpp)
target_link_libraries(${TEST_TARGET} cdt gtest gmock gtest_main)
