# This will create a static library that test code and python can reference

# add library
add_library(mesh2py_lib fbx2py/fbx_importer.cpp common/scene_data.cpp gltf2py/gltf_importer.cpp)

message(STATUS "SOURCE dir ${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(mesh2py_lib
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mesh2py_lib
PUBLIC
    ufbx::ufbx
)

compile_config(mesh2py_lib)

# Install the library
install(TARGETS mesh2py_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# TODO look into this later
# Install the export set
# install(EXPORT mesh2py_targets
#     FILE mesh2py_targets.cmake
#     NAMESPACE mesh2py::
#     DESTINATION lib/cmake/mesh2py
# )