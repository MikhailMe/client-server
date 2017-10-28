cmake_minimum_required(VERSION 3.8)

project(client-server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g2 -O0")

set(SF_FUNCTIONS functions.h)
set(SF_CONSTANTS constants.h)

set(SF_CLIENT client.cpp ${SF_FUNCTIONS} ${SF_CONSTANTS})
add_executable(client ${SF_CLIENT})

set(SF_MULTITHREADINGSERVER multithreading_server.cpp ${SF_FUNCTIONS} ${SF_CONSTANTS})
add_executable(multithreadingserver ${SF_MULTITHREADINGSERVER})