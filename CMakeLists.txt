cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

# create project
project(mesh2py)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# add dependencies
include(cmake/CPM.cmake)
include(cmake/compiler_config.cmake)

find_package(tinygltf REQUIRED)
find_package(ufbx REQUIRED)

add_subdirectory(src)
add_subdirectory(python)