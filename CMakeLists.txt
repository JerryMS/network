cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")

project (tcp-socket)

include_directories(include)

file(GLOB SOURCES "*.cpp")

add_executable(tcp-socket ${SOURCES})
