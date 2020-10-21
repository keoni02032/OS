cmake_minimum_required(VERSION 3.1)
project(lab6)

set(CMAKE_CXX_STANDARD 17)
add_executable(main handlerNode.cpp)
add_executable(calcNode calcNode.cpp)
add_library(sockets socketRoutine.cpp socketRoutine.hpp)

target_link_libraries(sockets zmq)
target_link_libraries(main zmq sockets)
target_link_libraries(calcNode zmq sockets)