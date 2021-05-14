cmake_minimum_required(VERSION 3.17)
project(ispc_in_cpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(ispc_in_cpp main.cpp include/varying.hpp include/control_flow.hpp)