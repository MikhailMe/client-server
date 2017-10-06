cmake_minimum_required(VERSION 3.8)

project(multiCS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g2 -O0")

set(SOURCE_FILES_FUNCTIONS functions.h)

set(SOURCE_FILES_MAIN main.cpp)
add_executable(main ${SOURCE_FILES_MAIN})

set(SOURCE_FILES_CLIENT client.cpp)
add_executable(client ${SOURCE_FILES_CLIENT})

set(SOURCE_FILES_MULTITHREADINGSERVER multithreading_server.cpp)
add_executable(multithreadingserver ${SOURCE_FILES_MULTITHREADINGSERVER})