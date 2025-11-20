cmake_minimum_required(VERSION 3.15)
project(MyCppApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

add_executable(MyCppApp src/main.cpp)

enable_testing()
add_subdirectory(tests)