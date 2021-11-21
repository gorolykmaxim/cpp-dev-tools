cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/tiny-process-library)
add_subdirectory(lib/cameron314_concurrentqueue)
add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/googletest)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC lib/tiny-process-library PUBLIC lib/nlohmann_json PUBLIC lib/cameron314_concurrentqueue)
target_link_libraries(${PROJECT_NAME} tiny-process-library nlohmann_json cameron314_concurrentqueue)

set(TEST_TARGET ${PROJECT_NAME}-test)
add_executable(${TEST_TARGET} test/test.cpp)
target_compile_definitions(${TEST_TARGET} PRIVATE "BINARY_DIR=\"${CMAKE_BINARY_DIR}\"" "BINARY_NAME=\"${PROJECT_NAME}\"")
add_dependencies(${TEST_TARGET} ${PROJECT_NAME})
target_include_directories(${TEST_TARGET} PUBLIC lib/tiny-process-library PUBLIC lib/cameron314_concurrentqueue)
target_link_libraries(${TEST_TARGET} gtest tiny-process-library cameron314_concurrentqueue gtest_main)
add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_SOURCE_DIR}/test/test-tasks.json
                   ${CMAKE_CURRENT_BINARY_DIR}/test-tasks.json)