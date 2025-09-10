cmake_minimum_required(VERSION 3.14)
project(hse-caos)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 20)
    message(WARNING "Using C++-20 instead of C++-23 because of old cmake (pre 3.20)")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include(cmake/Arch.cmake)
include(cmake/Helpers.cmake)
include(cmake/Flags.cmake)

add_subdirectory(common)
include_directories(common/include)

add_subdirectory(tasks)
