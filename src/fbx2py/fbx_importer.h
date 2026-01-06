#pragma once

#include <common/scene_data.h>

#include <unordered_map>
#include <ufbx.h>

mesh2py::common::SceneStorage ImportFbx(const char* path);