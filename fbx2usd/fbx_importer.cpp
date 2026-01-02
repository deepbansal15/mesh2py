#include "fbx_importer.h"

namespace mesh2usd::fbx2usd {

    void ImportMeshes(FbxContext* context, SceneView& scene_view) {
        auto mesh_list = context->scene->meshes;
        for (uint32_t i = 0; i < mesh_list.count; ++i) {
            MeshInfo& info = scene_view.meshes_info[i];
            ufbx_mesh* fbx_mesh = mesh_list[i];

            info.face_count = fbx_mesh->faces.count;
            uint32_t attrib_count = 0;
            attrib_count += fbx_mesh->vertex_position.exists ? 1 : 0;
            attrib_count += fbx_mesh->vertex_normal.exists ? 1 : 0;
            attrib_count += fbx_mesh->vertex_tangent.exists ? 1 : 0;
            attrib_count += fbx_mesh->vertex_bitangent.exists ? 1 : 0;
            attrib_count += fbx_mesh->uv_sets.count;
            attrib_count += fbx_mesh->color_sets.count;
            info.attribute_count = attrib_count;

            CalculateMeshOffsets(scene_view, context->storage);

            MeshView mesh_view = GetMeshView(context->storage, info);
            
            // Fill up start index for triangular faces
            for (uint32_t face_idx = 0; face_idx < info.face_count; ++face_idx) {
                mesh_view.face_start_index[face_idx] = fbx_mesh->faces[face_idx].index_begin;
            }

            // Fill up each attribute
            uint32_t attrib_idx = 0;
            
            // Position attribute
            if (fbx_mesh->vertex_position.exists) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::Position;
                attrib_info.index_count = fbx_mesh->vertex_position.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec3) / sizeof(float);
                attrib_info.data_count = fbx_mesh->vertex_position.values.count;
            }
            
            // Normal attribute
            if (fbx_mesh->vertex_normal.exists) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::Normal;
                attrib_info.index_count = fbx_mesh->vertex_normal.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec3) / sizeof(float);
                attrib_info.data_count = fbx_mesh->vertex_normal.values.count; // triangular faces
                
            }
            
            // Tangent attribute
            if (fbx_mesh->vertex_tangent.exists) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::Tangent;
                attrib_info.index_count = fbx_mesh->vertex_tangent.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec3) / sizeof(float);
                attrib_info.data_count = fbx_mesh->vertex_tangent.values.count; // triangular faces
            }
            
            // Bitangent attribute
            if (fbx_mesh->vertex_bitangent.exists) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::BiTangent;
                attrib_info.index_count = fbx_mesh->vertex_bitangent.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec3) / sizeof(float);
                attrib_info.data_count = fbx_mesh->vertex_bitangent.values.count; // triangular faces
            }
            
            // UV sets
            for (uint32_t uv_idx = 0; uv_idx < fbx_mesh->uv_sets.count; ++uv_idx) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::TexCoord;
                attrib_info.index_count = fbx_mesh->uv_sets[uv_idx].vertex_uv.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec2) / sizeof(float);
                attrib_info.data_count = fbx_mesh->uv_sets[uv_idx].vertex_uv.values.count;
            }
            
            // Color sets
            for (uint32_t color_idx = 0; color_idx < fbx_mesh->color_sets.count; ++color_idx) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx++];
                attrib_info.attrib_type = VertexAttribType::Color;
                attrib_info.index_count = fbx_mesh->color_sets[color_idx].vertex_color.indices.count;
                attrib_info.num_value_per_index = sizeof(ufbx_vec4) / sizeof(float);
                attrib_info.data_count = fbx_mesh->color_sets[color_idx].vertex_color.values.count;
            }

            // Calculate attribute offsets after setting up all attributes
            CalculateAttributeOffsets(mesh_view, context->storage);

            // Fill the vertex data here with memcpy
            uint32_t current_uv_idx = 0;
            uint32_t current_color_idx = 0;
            
            for (uint32_t attrib_idx = 0; attrib_idx < info.attribute_count; ++attrib_idx) {
                AttributeInfo& attrib_info = mesh_view.attribs_info[attrib_idx];
                AttributeView attrib_view = GetAttribView(context->storage, info, attrib_info);
                
                switch (attrib_info.attrib_type) {
                    case VertexAttribType::Position: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->vertex_position.indices.data, fbx_mesh->vertex_position.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->vertex_position.values.data, fbx_mesh->vertex_position.values.count * sizeof(ufbx_vec3));
                        break;
                    }
                    case VertexAttribType::Normal: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->vertex_normal.indices.data, fbx_mesh->vertex_normal.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->vertex_normal.values.data, fbx_mesh->vertex_normal.values.count * sizeof(ufbx_vec3));
                        break;
                    }
                    case VertexAttribType::Tangent: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->vertex_tangent.indices.data, fbx_mesh->vertex_tangent.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->vertex_tangent.values.data, fbx_mesh->vertex_tangent.values.count * sizeof(ufbx_vec3));
                        break;
                    }
                    case VertexAttribType::BiTangent: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->vertex_bitangent.indices.data, fbx_mesh->vertex_bitangent.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->vertex_bitangent.values.data, fbx_mesh->vertex_bitangent.values.count * sizeof(ufbx_vec3));
                        break;
                    }
                    case VertexAttribType::TexCoord: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->uv_sets[current_uv_idx].vertex_uv.indices.data, fbx_mesh->uv_sets[current_uv_idx].vertex_uv.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->uv_sets[current_uv_idx].vertex_uv.values.data, fbx_mesh->uv_sets[current_uv_idx].vertex_uv.values.count * sizeof(ufbx_vec2));
                        current_uv_idx++;
                        break;
                    }
                    case VertexAttribType::Color: {
                        memcpy(attrib_view.indices.data(), fbx_mesh->color_sets[current_color_idx].vertex_color.indices.data, fbx_mesh->color_sets[current_color_idx].vertex_color.indices.count * sizeof(uint32_t));
                        memcpy(attrib_view.data.data(), fbx_mesh->color_sets[current_color_idx].vertex_color.values.data, fbx_mesh->color_sets[current_color_idx].vertex_color.values.count * sizeof(ufbx_vec4));
                        current_color_idx++;
                        break;
                    }
                }
            }

            context->mesh_to_index[fbx_mesh] = i;
        }
    }

    void ImportNodes(FbxContext* context, SceneView& scene_view) {
        auto node_list = context->scene->nodes;
        
        // First pass: store all node pointers and initial mesh indices
        for (uint32_t i = 0; i < node_list.count; ++i) {
            ufbx_node* fbx_node = node_list[i];
            context->node_to_index[fbx_node] = i;
        }
        
        // Second pass: populate node data using the established mappings
        for (uint32_t i = 0; i < node_list.count; ++i) {
            Node& node = scene_view.nodes[i];
            ufbx_node* fbx_node = node_list[i];
            
            // Set parent index - convert parent pointer to index
            if (fbx_node->parent) {
                auto it = context->node_to_index.find(fbx_node->parent);
                if (it != context->node_to_index.end()) {
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
            
            // Copy transform matrix from FBX node
            // Using local_transform as the fundamental transformation
            memcpy(node.transform, &fbx_node->local_transform, sizeof(float) * 16);
            
            // Set mesh index - find associated mesh in the mesh_to_index map
            if (fbx_node->mesh) {
                auto it = context->mesh_to_index.find(fbx_node->mesh);
                if (it != context->mesh_to_index.end()) {
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

    void ImportScene(FbxContext* context) {
        SceneStorage& storage = context->storage;
        storage.scene_info.mesh_count = context->scene->meshes.count;
        storage.scene_info.node_count = context->scene->nodes.count;

        SceneView scene_view = GetSceneView(storage);
        ImportMeshes(context, scene_view);
        ImportNodes(context, scene_view);
    }
}