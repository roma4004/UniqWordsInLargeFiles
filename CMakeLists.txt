cmake_minimum_required(VERSION 3.23)
project(SubStrA)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_executable(SubStrA main.cpp testFileGenerator.cpp)
target_link_libraries(SubStrA Threads::Threads)
