cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME bspsieve)
set(BULK_PATH ${CMAKE_SOURCE_DIR}/../../Bulk/include/)
set(EXEC bspsieve_class.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER "g++-11")
set(CMAKE_C_COMPILER "gcc-11")

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} ${EXEC})
include_directories(${BULK_PATH})
target_link_libraries(${PROJECT_NAME} pthread)