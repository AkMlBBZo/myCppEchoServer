cmake_minimum_required(VERSION 3.15)
project(gochan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gochan INTERFACE)
target_include_directories(gochan INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# enable_testing()
# add_subdirectory(tests)