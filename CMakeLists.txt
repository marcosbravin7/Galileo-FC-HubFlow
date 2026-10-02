cmake_minimum_required(VERSION 3.20)
project(HubFlow)

set(CMAKE_CXX_STANDARD 17)

add_executable(HubFlow main.cpp)

add_executable(HubFlowTests tests/test_main.cpp)
target_include_directories(HubFlowTests PRIVATE ${CMAKE_SOURCE_DIR})
