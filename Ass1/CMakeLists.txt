cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME Hello_World)
set(BULK_PATH ${CMAKE_SOURCE_DIR}/../../Bulk/include/)
set(EXEC parallel_sieve.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER "g++-11")
set(CMAKE_C_COMPILER "gcc-11")

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} ${EXEC})
include_directories(${BULK_PATH})
target_link_libraries(${PROJECT_NAME} pthread)