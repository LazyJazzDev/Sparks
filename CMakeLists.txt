cmake_minimum_required(VERSION 3.22)

project(Sparks)

set(CMAKE_CXX_STANDARD 17)

set(SPARKS_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/)

add_subdirectory(external/LongMarch)

add_subdirectory(code)
