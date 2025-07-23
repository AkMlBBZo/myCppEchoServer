cmake_minimum_required(VERSION 3.15)
project(cppProjects LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(logger)
add_subdirectory(gochan)
add_subdirectory(ClientManager)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE
    logger
    ClientManager
)

# paths
set(LOGS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/logs")
set(MAIN_LOG_FILE "${LOGS_DIR}/logs.txt")
set(CLIENT_MANAGER_LOG_FILE "${LOGS_DIR}/ClientManagerLogs.txt")

# config.h gen
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
    @ONLY
)

# adding bin dir to include-paths
include_directories(${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(ClientManager PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
