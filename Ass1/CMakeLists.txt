cmake_minimum_required(VERSION 3.16)

project(Hello_World)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/bin/g++-11)
set(CMAKE_C_COMPILER /usr/bin/gcc-11)
set(BULK_PATH "../Bulk/include")

add_executable(Hello_World hello.cpp)
target_include_directories(Hello_World PUBLIC BULK_PATH)
target_link_libraries(Hello_World pthread)