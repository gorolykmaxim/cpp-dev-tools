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
add_subdirectory(lib/googletest)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC lib/tiny-process-library PUBLIC lib/nlohmann_json PUBLIC lib/cameron314_concurrentqueue)
target_link_libraries(${PROJECT_NAME} tiny-process-library nlohmann_json cameron314_concurrentqueue)

set(TEST_TARGET ${PROJECT_NAME}-test)
add_executable(${TEST_TARGET} test/test.cpp)
target_compile_definitions(${TEST_TARGET} PRIVATE
    "BINARY_DIR=\"${CMAKE_BINARY_DIR}\""
    "BINARY_NAME=\"${PROJECT_NAME}\""
    "TEST_CONFIGS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test/test-configs\""
    "TEST_HOMES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test/test-homes\"")
add_dependencies(${TEST_TARGET} ${PROJECT_NAME})
target_include_directories(${TEST_TARGET} PUBLIC lib/tiny-process-library PUBLIC lib/cameron314_concurrentqueue)
target_link_libraries(${TEST_TARGET} gtest tiny-process-library cameron314_concurrentqueue gtest_main)
