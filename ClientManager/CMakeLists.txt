cmake_minimum_required(VERSION 3.15)

add_library(ClientManager
    src/ClientManager.cpp
    include/ClientManager.h
)

target_link_libraries(ClientManager PRIVATE logger)
if (WIN32)
    target_link_libraries(ClientManager PRIVATE ws2_32) # winsock2
endif()

target_include_directories(ClientManager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../logger/include
)