# Find Python and nanobind

set(MODULE_TARGET_NAME mesh2py)
# Create the Python module
nanobind_add_module(
    ${MODULE_TARGET_NAME}
    NB_STATIC
    mesh2py_bindings.cpp
)

target_compile_features(${MODULE_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${MODULE_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(${MODULE_TARGET_NAME} PRIVATE NB_MODULE_NAME=${MODULE_TARGET_NAME})
target_link_libraries(${MODULE_TARGET_NAME} PRIVATE mesh2py_lib)
target_include_directories(${MODULE_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

install(TARGETS ${MODULE_TARGET_NAME} LIBRARY DESTINATION ${SKBUILD_PROJECT_NAME})
# Need to know how I can use SKBuild_PROJECT_NAME
#install(TARGETS ${MODULE_TARGET_NAME} LIBRARY DESTINATION .)

if(MSVC)
  target_compile_options(${MODULE_TARGET_NAME} PRIVATE /W4)
else()
  target_compile_options(${MODULE_TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Wno-error)
endif()