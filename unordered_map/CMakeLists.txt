cmake_minimum_required(VERSION 3.9)
project(hash_map)

set(CMAKE_CXX_STANDARD 11)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread" )


add_executable(hash_map main.cpp)