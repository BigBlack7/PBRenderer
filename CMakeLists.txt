cmake_minimum_required(VERSION 3.20)
project(PBRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(BUILD_SHARED_LIBS OFF)

# compiler options
if(MSVC)
    cmake_policy(SET CMP0077 NEW)
    add_compile_options(/MP)
endif()

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(thirdParty)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
add_subdirectory(core)
add_subdirectory(samples)