#pragma once


#include "../scene_data.h"

#include <unordered_map>

#include <ufbx.h>

namespace mesh2py::fbx {

struct FbxContext {
    const ufbx_scene* scene;
    std::unordered_map<ufbx_node*, int> node_to_index;
    std::unordered_map<ufbx_mesh*, int> mesh_to_index;
    SceneStorage storage;
};

void ImportScene(FbxContext& context);
}