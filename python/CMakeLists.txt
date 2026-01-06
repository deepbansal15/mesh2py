cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

# Find Python and nanobind
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# Create the Python module
nanobind_add_module(fbx_importer
    fbx_importer_bindings.cpp
)

# Link against the mesh2py library
target_link_libraries(fbx_importer PRIVATE mesh2py_lib)

# Set include directories
target_include_directories(fbx_importer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Set C++ standard
set_target_properties(fbx_importer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Install the Python module
install(TARGETS fbx_importer
    DESTINATION .
)