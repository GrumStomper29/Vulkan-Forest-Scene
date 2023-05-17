cmake_minimum_required(VERSION 3.26.3)

project(Forest_Scene)

set(CMAKE_CXX_STANDARD 20)
set(BINARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(third_party)
add_subdirectory(src)