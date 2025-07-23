cmake_minimum_required(VERSION 3.15)
project(Logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(logger INTERFACE)

target_include_directories(logger INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../gochan/include
)

target_link_libraries(logger INTERFACE 
    stdc++fs
    Threads::Threads
)

# enable_testing()
# add_subdirectory(tests)