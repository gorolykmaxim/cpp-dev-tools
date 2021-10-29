cmake_minimum_required(VERSION 3.16)
project(cpp-dev-tools)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/tiny-process-library)
add_subdirectory(lib/nlohmann-json)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC lib/tiny-process-library PUBLIC lib/nlohmann_json)
target_link_libraries(${PROJECT_NAME} tiny-process-library nlohmann_json)