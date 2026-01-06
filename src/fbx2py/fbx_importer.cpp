#include "fbx_importer.h"

namespace mesh2py::fbx {
    using namespace mesh2py::common;

    struct FbxContext {
        const ufbx_scene* scene;
        std::unordered_map<ufbx_node*, int> node_to_index;
        std::unordered_map<ufbx_mesh*, int> mesh_to_index;
        SceneStorage storage;
    };

    // Helper function to copy indices (no conversion needed - they're uint32_t)
    inline void CopyIndices(uint32_t* dst, const uint32_t* src, size_t count) {
        memcpy(dst, src, count * sizeof(uint32_t));
    }

    // Helper to convert vec2 array from double to float
    inline void ConvertVec2ToFloat(float* dst, const ufbx_vec2* src, size_t count) {
        for (size_t i = 0; i < count; i++) {
            dst[i * 2 + 0] = static_cast<float>(src[i].x);
            dst[i * 2 + 1] = static_cast<float>(src[i].y);
        }
    }

    // Helper to convert vec3 array from double to float
    inline void ConvertVec3ToFloat(float* dst, const ufbx_vec3* src, size_t count) {
        for (size_t i = 0; i < count; i++) {
            dst[i * 3 + 0] = static_cast<float>(src[i].x);
            dst[i * 3 + 1] = static_cast<float>(src[i].y);
            dst[i * 3 + 2] = static_cast<float>(src[i].z);
        }
    }

    // Helper to convert vec4 array from double to float
    inline void ConvertVec4ToFloat(float* dst, const ufbx_vec4* src, size_t count) {
        for (size_t i = 0; i < count; i++) {
            dst[i * 4 + 0] = static_cast<float>(src[i].x);
            dst[i * 4 + 1] = static_cast<float>(src[i].y);
            dst[i * 4 + 2] = static_cast<float>(src[i].z);
            dst[i * 4 + 3] = static_cast<float>(src[i].w);
        }
    }

static void ImportMesh(SceneStorage& storage, MeshInfo& mesh_info, ufbx_mesh* fbx_mesh) {
    
    // Import faces first
    FaceView view = GetFaceView(storage, mesh_info);
    memcpy(view.faces.data(), fbx_mesh->faces.data, sizeof(ufbx_face) * mesh_info.face_count);
    
    // Import all the vertex attributes
    uint32_t current_uv_idx = 0;
    uint32_t current_color_idx = 0;

    uint32_t attrib_start_index = mesh_info.attrib_info_start_index;
    uint32_t attrib_end_index = attrib_start_index + mesh_info.attribute_info_count;
    
    for (uint32_t attrib_idx = attrib_start_index; attrib_idx < attrib_end_index; ++attrib_idx) {
        AttributeInfo& attrib_info = storage.attrib_infos[attrib_idx];
        AttributeView attrib_view = GetAttribView(storage, attrib_info);
                
        switch (attrib_info.attrib_type) {
            case VertexAttribType::Position: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->vertex_position.indices.data, fbx_mesh->vertex_position.indices.count);
                ConvertVec3ToFloat(attrib_view.data.data(), fbx_mesh->vertex_position.values.data, fbx_mesh->vertex_position.values.count);
                break;
            }
            case VertexAttribType::Normal: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->vertex_normal.indices.data, fbx_mesh->vertex_normal.indices.count);
                ConvertVec3ToFloat(attrib_view.data.data(), fbx_mesh->vertex_normal.values.data, fbx_mesh->vertex_normal.values.count);
                break;
            }
            case VertexAttribType::Tangent: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->vertex_tangent.indices.data, fbx_mesh->vertex_tangent.indices.count);
                ConvertVec3ToFloat(attrib_view.data.data(), fbx_mesh->vertex_tangent.values.data, fbx_mesh->vertex_tangent.values.count);
                break;
            }
            case VertexAttribType::BiTangent: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->vertex_bitangent.indices.data, fbx_mesh->vertex_bitangent.indices.count);
                ConvertVec3ToFloat(attrib_view.data.data(), fbx_mesh->vertex_bitangent.values.data, fbx_mesh->vertex_bitangent.values.count);
                break;
            }
            case VertexAttribType::TexCoord: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->uv_sets[current_uv_idx].vertex_uv.indices.data, fbx_mesh->uv_sets[current_uv_idx].vertex_uv.indices.count);
                ConvertVec2ToFloat(attrib_view.data.data(), fbx_mesh->uv_sets[current_uv_idx].vertex_uv.values.data, fbx_mesh->uv_sets[current_uv_idx].vertex_uv.values.count);
                current_uv_idx++;
                break;
            }
            case VertexAttribType::Color: {
                CopyIndices(attrib_view.indices.data(), fbx_mesh->color_sets[current_color_idx].vertex_color.indices.data, fbx_mesh->color_sets[current_color_idx].vertex_color.indices.count);
                ConvertVec4ToFloat(attrib_view.data.data(), fbx_mesh->color_sets[current_color_idx].vertex_color.values.data, fbx_mesh->color_sets[current_color_idx].vertex_color.values.count);
                current_color_idx++;
                break;
            }
        }
    }

}

void ImportMeshes(FbxContext& context) {
    SceneStorage& storage = context.storage;
    for (uint32_t i = 0; i < context.scene->meshes.count; ++i) {
        ufbx_mesh* fbx_mesh = context.scene->meshes[i];
        MeshInfo& mesh_info = storage.mesh_infos[i];
        ImportMesh(storage, mesh_info, fbx_mesh);
            
        context.mesh_to_index[fbx_mesh] = i;
    }
}

void ImportNodes(FbxContext& context) {
    auto node_list = context.scene->nodes;
    
    // First pass: store all node pointers and initial mesh indices
    for (uint32_t i = 0; i < node_list.count; ++i) {
        ufbx_node* fbx_node = node_list[i];
        context.node_to_index[fbx_node] = i;
    }
    
    SceneStorage& storage = context.storage;
    
    // Second pass: populate node data using the established mappings
    for (uint32_t i = 0; i < node_list.count; ++i) {
        Node& node = storage.nodes[i];
        ufbx_node* fbx_node = node_list[i];
        
        // Set parent index - convert parent pointer to index
        if (fbx_node->parent) {
            auto it = context.node_to_index.find(fbx_node->parent);
            if (it != context.node_to_index.end()) {
                node.parent = it->second;
            } else {
                // Parent node not found in the scene's nodes list
                // This can happen if the parent is a structural node not in the main nodes array
                node.parent = UINT32_MAX;
            }
        } else {
            // Root node (no parent)
            node.parent = UINT32_MAX;
        }
        
        // Copy transform matrix from FBX node element-wise from double to float
        // Using local_transform as the fundamental transformation
        const ufbx_real* src_matrix = &fbx_node->local_transform.translation.x;
        for (uint32_t j = 0; j < 16; j++) {
            node.transform[j] = static_cast<float>(src_matrix[j]);
        }
        
        // Set mesh index - find associated mesh in the mesh_to_index map
        if (fbx_node->mesh) {
            auto it = context.mesh_to_index.find(fbx_node->mesh);
            if (it != context.mesh_to_index.end()) {
                node.mesh_index = it->second;
            } else {
                // Associated mesh not found (shouldn't happen if ImportMeshes was called first)
                node.mesh_index = UINT32_MAX;
            }
        } else {
            // No mesh associated with this node
            node.mesh_index = UINT32_MAX;
        }
    }
}

template<typename T>
static uint32_t AllocateAttribute(SceneStorage& storage, T& vertex_attrib_data,
    uint32_t current_offset, uint32_t attrib_index, VertexAttribType attrib_type,
    uint32_t num_value_per_index) {
    AttributeInfo& attrib_info = storage.attrib_infos[attrib_index];
    attrib_info.attrib_type = attrib_type;
    
    attrib_info.index_offset = align_up(current_offset, 16);
    attrib_info.index_count = vertex_attrib_data.indices.count;
    attrib_info.num_value_per_index = num_value_per_index;

    current_offset = attrib_info.index_offset + attrib_info.index_count * sizeof(uint32_t);
    attrib_info.value_offset = align_up(current_offset, 16);
    attrib_info.value_count = vertex_attrib_data.values.count;
    
    current_offset = attrib_info.value_offset + attrib_info.value_count * sizeof(float) * attrib_info.num_value_per_index;
    return current_offset;
}

void AllocateSceneData(FbxContext& context) {
    SceneStorage& storage = context.storage;
    uint32_t current_offset = 0;
    
    storage.nodes.resize(context.scene->nodes.count);
    storage.mesh_infos.resize(context.scene->meshes.count);
    for (uint32_t mesh_idx = 0; mesh_idx < storage.mesh_infos.size(); ++mesh_idx) {
        MeshInfo& mesh_info = storage.mesh_infos[mesh_idx];
        ufbx_mesh* fbx_mesh = context.scene->meshes[mesh_idx];
        mesh_info.face_offset = align_up(current_offset, 16);
        mesh_info.face_count = fbx_mesh->faces.count;
        current_offset = mesh_info.face_offset + mesh_info.face_count * sizeof(ufbx_face);
        
        // Fill up attributes
        uint32_t attrib_count = 0;
        attrib_count += fbx_mesh->vertex_position.exists ? 1 : 0;
        attrib_count += fbx_mesh->vertex_normal.exists ? 1 : 0;
        attrib_count += fbx_mesh->vertex_tangent.exists ? 1 : 0;
        attrib_count += fbx_mesh->vertex_bitangent.exists ? 1 : 0;
        attrib_count += fbx_mesh->uv_sets.count;
        attrib_count += fbx_mesh->color_sets.count;
        
        mesh_info.attribute_info_count = attrib_count;
        mesh_info.attrib_info_start_index = storage.attrib_infos.size();
        storage.attrib_infos.resize(storage.attrib_infos.size() + attrib_count);
        // Fill up each attribute
        uint32_t attrib_idx = mesh_info.attrib_info_start_index;
        // Position attribute
        if (fbx_mesh->vertex_position.exists) {
            current_offset = AllocateAttribute(storage, fbx_mesh->vertex_position, current_offset, 
                attrib_idx, VertexAttribType::Position, 3);
            attrib_idx++;
        }
        
        // Normal attribute
        if (fbx_mesh->vertex_normal.exists) {
            current_offset = AllocateAttribute(storage, fbx_mesh->vertex_normal, current_offset, 
                attrib_idx, VertexAttribType::Normal, 3);
            attrib_idx++;
        }
        
        // Tangent attribute
        if (fbx_mesh->vertex_tangent.exists) {
            current_offset = AllocateAttribute(storage, fbx_mesh->vertex_tangent, current_offset, 
                attrib_idx, VertexAttribType::Tangent, 3);
            attrib_idx++;
        }
        
        // Bitangent attribute
        if (fbx_mesh->vertex_bitangent.exists) {
            current_offset = AllocateAttribute(storage, fbx_mesh->vertex_bitangent, current_offset, 
                attrib_idx, VertexAttribType::BiTangent, 3);
            attrib_idx++;
        }
        
        // UV sets
        for (uint32_t uv_idx = 0; uv_idx < fbx_mesh->uv_sets.count; ++uv_idx) {
            current_offset = AllocateAttribute(storage, fbx_mesh->uv_sets[uv_idx].vertex_uv, current_offset, 
                attrib_idx, VertexAttribType::TexCoord, 2);
            attrib_idx++;
        }
        
        // Color sets
        for (uint32_t color_idx = 0; color_idx < fbx_mesh->color_sets.count; ++color_idx) {
            current_offset = AllocateAttribute(storage, fbx_mesh->color_sets[color_idx].vertex_color, current_offset, 
                attrib_idx, VertexAttribType::Color, 4);
            attrib_idx++;
        }
    }
    storage.data.resize(current_offset);
}

void ImportScene(FbxContext& context) {
    AllocateSceneData(context);
    ImportMeshes(context);
    ImportNodes(context);
}

}

mesh2py::common::SceneStorage ImportFbx(const char* path) {
    ufbx_load_opts load_opts = {};
    ufbx_error error = {};
    
    ufbx_scene* scene = ufbx_load_file(path, &load_opts, &error);
    if (!scene) {
        printf("Error %s/n", error.description.data);
        return {};
    }
    
    mesh2py::fbx::FbxContext context;
    context.scene = scene;
    mesh2py::fbx::ImportScene(context);

    ufbx_free_scene(scene);

    return context.storage;
}