#include "fbx_importer.h"

#include <ufbx.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <cmath>

#include "obj_writer.h"

namespace mesh2py::fbxtest {
using namespace mesh2py;
using namespace mesh2py::fbx;

// ============================================================================
// Helper Functions for Value Comparison
// ============================================================================

// Compare uint32_t values
bool CompareUint32(uint32_t expected, uint32_t actual, const char* context) {
    if (expected != actual) {
        std::cerr << "Mismatch in " << context << ": expected=" << expected
                  << ", actual=" << actual << std::endl;
        return false;
    }
    return true;
}


// Compare ufbx_indices arrays - ufbx uses void* for data, cast to uint32_t*
bool CompareUfbxIndexArrays(const uint32_t* expected_indices, const uint32_t* actual,
                            size_t expected_count, const char* context) {
    for (size_t i = 0; i < expected_count; ++i) {
        uint32_t expected_idx = expected_indices[i];
        uint32_t actual_idx = actual[i];
        if (expected_idx != actual_idx) {
            std::cerr << "Mismatch in " << context << "[" << i << "]: expected="
                      << expected_idx << ", actual=" << actual_idx << std::endl;
            return false;
        }
    }
    return true;
}

// Compare float values (ufbx uses double, SceneStorage uses float)
bool CompareFloat(double expected_double, float actual_float, const char* context) {
    float expected_float = static_cast<float>(expected_double);
    if (expected_float != actual_float) {
        std::cerr << "Mismatch in " << context << ": expected=" << expected_float
                  << " (from double " << expected_double << "), actual=" << actual_float << std::endl;
        return false;
    }
    return true;
}

// Compare ufbx_vec2 arrays (x, y) with SceneStorage float arrays (2 floats per vec2)
bool CompareVec2Arrays(const ufbx_vec2* expected_vec2, const float* actual_float,
                       size_t count, const char* context) {
    for (size_t i = 0; i < count; ++i) {
        float expected_x = static_cast<float>(expected_vec2[i].x);
        float expected_y = static_cast<float>(expected_vec2[i].y);
        float actual_x = actual_float[i * 2 + 0];
        float actual_y = actual_float[i * 2 + 1];
        
        if (expected_x != actual_x) {
            std::cerr << "Mismatch in " << context << "[" << i << "].x: expected="
                      << expected_x << ", actual=" << actual_x << std::endl;
            return false;
        }
        if (expected_y != actual_y) {
            std::cerr << "Mismatch in " << context << "[" << i << "].y: expected="
                      << expected_y << ", actual=" << actual_y << std::endl;
            return false;
        }
    }
    return true;
}

// Compare ufbx_vec3 arrays (x, y, z) with SceneStorage float arrays (3 floats per vec3)
bool CompareVec3Arrays(const ufbx_vec3* expected_vec3, const float* actual_float,
                       size_t count, const char* context) {
    for (size_t i = 0; i < count; ++i) {
        float expected_x = static_cast<float>(expected_vec3[i].x);
        float expected_y = static_cast<float>(expected_vec3[i].y);
        float expected_z = static_cast<float>(expected_vec3[i].z);
        float actual_x = actual_float[i * 3 + 0];
        float actual_y = actual_float[i * 3 + 1];
        float actual_z = actual_float[i * 3 + 2];
        
        if (expected_x != actual_x) {
            std::cerr << "Mismatch in " << context << "[" << i << "].x: expected="
                      << expected_x << ", actual=" << actual_x << std::endl;
            return false;
        }
        if (expected_y != actual_y) {
            std::cerr << "Mismatch in " << context << "[" << i << "].y: expected="
                      << expected_y << ", actual=" << actual_y << std::endl;
            return false;
        }
        if (expected_z != actual_z) {
            std::cerr << "Mismatch in " << context << "[" << i << "].z: expected="
                      << expected_z << ", actual=" << actual_z << std::endl;
            return false;
        }
    }
    return true;
}

// Compare ufbx_vec4 arrays (x, y, z, w) with SceneStorage float arrays (4 floats per vec4)
bool CompareVec4Arrays(const ufbx_vec4* expected_vec4, const float* actual_float,
                       size_t count, const char* context) {
    for (size_t i = 0; i < count; ++i) {
        float expected_x = static_cast<float>(expected_vec4[i].x);
        float expected_y = static_cast<float>(expected_vec4[i].y);
        float expected_z = static_cast<float>(expected_vec4[i].z);
        float expected_w = static_cast<float>(expected_vec4[i].w);
        float actual_x = actual_float[i * 4 + 0];
        float actual_y = actual_float[i * 4 + 1];
        float actual_z = actual_float[i * 4 + 2];
        float actual_w = actual_float[i * 4 + 3];
        
        if (expected_x != actual_x) {
            std::cerr << "Mismatch in " << context << "[" << i << "].x: expected="
                      << expected_x << ", actual=" << actual_x << std::endl;
            return false;
        }
        if (expected_y != actual_y) {
            std::cerr << "Mismatch in " << context << "[" << i << "].y: expected="
                      << expected_y << ", actual=" << actual_y << std::endl;
            return false;
        }
        if (expected_z != actual_z) {
            std::cerr << "Mismatch in " << context << "[" << i << "].z: expected="
                      << expected_z << ", actual=" << actual_z << std::endl;
            return false;
        }
        if (expected_w != actual_w) {
            std::cerr << "Mismatch in " << context << "[" << i << "].w: expected="
                      << expected_w << ", actual=" << actual_w << std::endl;
            return false;
        }
    }
    return true;
}

// ============================================================================
// Face Verification
// ============================================================================

bool VerifyFaces(const ufbx_scene* scene, SceneStorage& storage) {
    std::cout << "Verifying faces..." << std::endl;
    bool all_passed = true;
    
    for (uint32_t mesh_idx = 0; mesh_idx < storage.mesh_infos.size(); ++mesh_idx) {
        MeshInfo& mesh_info = storage.mesh_infos[mesh_idx];
        ufbx_mesh* fbx_mesh = scene->meshes[mesh_idx];
        
        FaceView face_view = GetFaceView(storage, mesh_info);
        
        for (uint32_t face_idx = 0; face_idx < mesh_info.face_count; ++face_idx) {
            Face& face = face_view.faces[face_idx];
            ufbx_face& fbx_face = fbx_mesh->faces[face_idx];
            
            char context[128];
            snprintf(context, sizeof(context), "Mesh %u Face %u indices_begin", mesh_idx, face_idx);
            if (!CompareUint32(fbx_face.index_begin, face.indices_begin, context)) {
                all_passed = false;
            }
            
            snprintf(context, sizeof(context), "Mesh %u Face %u num_of_indices", mesh_idx, face_idx);
            if (!CompareUint32(fbx_face.num_indices, face.num_of_indices, context)) {
                all_passed = false;
            }
        }
    }
    
    if (all_passed) {
        std::cout << "  All faces verified successfully" << std::endl;
    }
    return all_passed;
}

// ============================================================================
// Attribute Indices Verification
// ============================================================================

bool VerifyAttributeIndices(const ufbx_mesh* fbx_mesh,
                           SceneStorage& storage,
                           MeshInfo& mesh_info,
                           uint32_t mesh_index) {
    bool all_passed = true;
    uint32_t uv_idx = 0;
    uint32_t color_idx = 0;
    
    for (uint32_t attrib_idx = mesh_info.attrib_info_start_index;
         attrib_idx < mesh_info.attrib_info_start_index + mesh_info.attribute_info_count;
         ++attrib_idx) {
        AttributeInfo& attrib_info = storage.attrib_infos[attrib_idx];
        AttributeView attrib_view = GetAttribView(storage, attrib_info);
        
        char context[128];
        
        switch (attrib_info.attrib_type) {
            case VertexAttribType::Position: {
                snprintf(context, sizeof(context), "Mesh %u Position indices", mesh_index);
                if (!CompareUfbxIndexArrays(fbx_mesh->vertex_position.indices.data,
                                            attrib_view.indices.data(),
                                            fbx_mesh->vertex_position.indices.count,
                                            context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::Normal: {
                snprintf(context, sizeof(context), "Mesh %u Normal indices", mesh_index);
                if (!CompareUfbxIndexArrays(fbx_mesh->vertex_normal.indices.data,
                                            attrib_view.indices.data(), 
                                            fbx_mesh->vertex_normal.indices.count,
                                            context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::Tangent: {
                snprintf(context, sizeof(context), "Mesh %u Tangent indices", mesh_index);
                if (!CompareUfbxIndexArrays(fbx_mesh->vertex_tangent.indices.data,
                                            attrib_view.indices.data(),
                                            fbx_mesh->vertex_tangent.indices.count,
                                            context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::BiTangent: {
                snprintf(context, sizeof(context), "Mesh %u BiTangent indices", mesh_index);
                if (!CompareUfbxIndexArrays(fbx_mesh->vertex_bitangent.indices.data,
                                            attrib_view.indices.data(),
                                            fbx_mesh->vertex_bitangent.indices.count,
                                            context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::TexCoord: {
                snprintf(context, sizeof(context), "Mesh %u TexCoord[%u] indices", mesh_index, uv_idx);
                if (!CompareUfbxIndexArrays(fbx_mesh->uv_sets[uv_idx].vertex_uv.indices.data,
                                            attrib_view.indices.data(),
                                            fbx_mesh->uv_sets[uv_idx].vertex_uv.indices.count,
                                            context)) {
                    all_passed = false;
                }
                uv_idx++;
                break;
            }
            case VertexAttribType::Color: {
                snprintf(context, sizeof(context), "Mesh %u Color[%u] indices", mesh_index, color_idx);
                if (!CompareUfbxIndexArrays(fbx_mesh->color_sets[color_idx].vertex_color.indices.data,
                                            attrib_view.indices.data(),
                                            fbx_mesh->color_sets[color_idx].vertex_color.indices.count,
                                            context)) {
                    all_passed = false;
                }
                color_idx++;
                break;
            }
            default:
                std::cerr << "Unknown attribute type: " << static_cast<uint32_t>(attrib_info.attrib_type) << std::endl;
                all_passed = false;
                break;
        }
    }
    
    return all_passed;
}

// ============================================================================
// Attribute Values Verification
// ============================================================================

bool VerifyAttributeValues(const ufbx_mesh* fbx_mesh,
                          SceneStorage& storage,
                          MeshInfo& mesh_info,
                          uint32_t mesh_index) {
    bool all_passed = true;
    uint32_t uv_idx = 0;
    uint32_t color_idx = 0;
    
    for (uint32_t attrib_idx = mesh_info.attrib_info_start_index;
         attrib_idx < mesh_info.attrib_info_start_index + mesh_info.attribute_info_count;
         ++attrib_idx) {
        AttributeInfo& attrib_info = storage.attrib_infos[attrib_idx];
        AttributeView attrib_view = GetAttribView(storage, attrib_info);
        
        char context[128];
        
        switch (attrib_info.attrib_type) {
            case VertexAttribType::Position: {
                
                break;
            }
            case VertexAttribType::Normal: {
                snprintf(context, sizeof(context), "Mesh %u Normal values", mesh_index);
                if (!CompareVec3Arrays(fbx_mesh->vertex_normal.values.data,
                                       attrib_view.data.data(),
                                       fbx_mesh->vertex_normal.values.count, context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::Tangent: {
                snprintf(context, sizeof(context), "Mesh %u Tangent values", mesh_index);
                if (!CompareVec3Arrays(fbx_mesh->vertex_tangent.values.data,
                                       attrib_view.data.data(),
                                       fbx_mesh->vertex_tangent.values.count, context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::BiTangent: {
                snprintf(context, sizeof(context), "Mesh %u BiTangent values", mesh_index);
                if (!CompareVec3Arrays(fbx_mesh->vertex_bitangent.values.data,
                                       attrib_view.data.data(),
                                       fbx_mesh->vertex_bitangent.values.count, context)) {
                    all_passed = false;
                }
                break;
            }
            case VertexAttribType::TexCoord: {
                snprintf(context, sizeof(context), "Mesh %u TexCoord[%u] values", mesh_index, uv_idx);
                if (!CompareVec2Arrays(fbx_mesh->uv_sets[uv_idx].vertex_uv.values.data,
                                        attrib_view.data.data(),
                                        fbx_mesh->uv_sets[uv_idx].vertex_uv.values.count, context)) {
                    all_passed = false;
                }
                uv_idx++;
                break;
            }
            case VertexAttribType::Color: {
                snprintf(context, sizeof(context), "Mesh %u Color[%u] values", mesh_index, color_idx);
                if (!CompareVec4Arrays(fbx_mesh->color_sets[color_idx].vertex_color.values.data,
                                        attrib_view.data.data(),
                                        fbx_mesh->color_sets[color_idx].vertex_color.values.count, context)) {
                    all_passed = false;
                }
                color_idx++;
                break;
            }
            default:
                break;
        }
    }
    
    return all_passed;
}

// ============================================================================
// All Attributes Verification (Indices + Values)
// ============================================================================

bool VerifyAllAttributes(const ufbx_scene* scene, SceneStorage& storage) {
    std::cout << "Verifying attribute indices and values..." << std::endl;
    bool all_passed = true;
    
    for (uint32_t mesh_idx = 0; mesh_idx < storage.mesh_infos.size(); ++mesh_idx) {
        MeshInfo& mesh_info = storage.mesh_infos[mesh_idx];
        ufbx_mesh* fbx_mesh = scene->meshes[mesh_idx];
        
        if (!VerifyAttributeIndices(fbx_mesh, storage, mesh_info, mesh_idx)) {
            all_passed = false;
        }
        
        if (!VerifyAttributeValues(fbx_mesh, storage, mesh_info, mesh_idx)) {
            all_passed = false;
        }
    }
    
    if (all_passed) {
        std::cout << "  All attributes verified successfully" << std::endl;
    }
    return all_passed;
}

// ============================================================================
// Node Transforms Verification
// ============================================================================

bool VerifyNodeTransforms(const ufbx_scene* scene, SceneStorage& storage) {
    std::cout << "Verifying node transforms..." << std::endl;
    bool all_passed = true;
    
    for (uint32_t node_idx = 0; node_idx < storage.nodes.size(); ++node_idx) {
        Node& node = storage.nodes[node_idx];
        ufbx_node* fbx_node = scene->nodes[node_idx];
        
        const ufbx_real* src_matrix = &fbx_node->local_transform.translation.x;
        
        for (uint32_t j = 0; j < 16; ++j) {
            char context[128];
            snprintf(context, sizeof(context), "Node %u transform[%u]", node_idx, j);
            
            double expected_double = src_matrix[j];
            float expected_float = static_cast<float>(expected_double);
            float actual_float = node.transform[j];
            
            if (expected_float != actual_float) {
                std::cerr << "Mismatch in " << context << ": expected=" << expected_float
                          << " (from double " << expected_double << "), actual=" << actual_float << std::endl;
                all_passed = false;
            }
        }
    }
    
    if (all_passed) {
        std::cout << "  All node transforms verified successfully" << std::endl;
    }
    return all_passed;
}

// ============================================================================
// Node Hierarchy Verification
// ============================================================================

bool VerifyNodeHierarchy(const ufbx_scene* scene, SceneStorage& storage) {
    std::cout << "Verifying node hierarchy..." << std::endl;
    bool all_passed = true;
    
    // Build expected parent index map from ufbx scene
    std::unordered_map<ufbx_node*, uint32_t> node_to_index;
    for (uint32_t i = 0; i < scene->nodes.count; ++i) {
        node_to_index[scene->nodes[i]] = i;
    }
    
    for (uint32_t node_idx = 0; node_idx < storage.nodes.size(); ++node_idx) {
        Node& node = storage.nodes[node_idx];
        ufbx_node* fbx_node = scene->nodes[node_idx];
        
        uint32_t expected_parent = UINT32_MAX;
        if (fbx_node->parent) {
            auto it = node_to_index.find(fbx_node->parent);
            if (it != node_to_index.end()) {
                expected_parent = it->second;
            }
        }
        
        char context[128];
        snprintf(context, sizeof(context), "Node %u parent", node_idx);
        
        if (!CompareUint32(expected_parent, node.parent, context)) {
            all_passed = false;
        }
    }
    
    if (all_passed) {
        std::cout << "  All node hierarchy verified successfully" << std::endl;
    }
    return all_passed;
}

// ============================================================================
// Mesh Associations Verification
// ============================================================================

bool VerifyMeshAssociations(const ufbx_scene* scene, SceneStorage& storage) {
    std::cout << "Verifying mesh associations..." << std::endl;
    bool all_passed = true;
    
    // Build expected mesh index map from ufbx scene
    std::unordered_map<ufbx_mesh*, uint32_t> mesh_to_index;
    for (uint32_t i = 0; i < scene->meshes.count; ++i) {
        mesh_to_index[scene->meshes[i]] = i;
    }
    
    for (uint32_t node_idx = 0; node_idx < storage.nodes.size(); ++node_idx) {
        Node& node = storage.nodes[node_idx];
        ufbx_node* fbx_node = scene->nodes[node_idx];
        
        uint32_t expected_mesh_index = UINT32_MAX;
        if (fbx_node->mesh) {
            auto it = mesh_to_index.find(fbx_node->mesh);
            if (it != mesh_to_index.end()) {
                expected_mesh_index = it->second;
            }
        }
        
        char context[128];
        snprintf(context, sizeof(context), "Node %u mesh_index", node_idx);
        
        if (!CompareUint32(expected_mesh_index, node.mesh_index, context)) {
            all_passed = false;
        }
    }
    
    if (all_passed) {
        std::cout << "  All mesh associations verified successfully" << std::endl;
    }
    return all_passed;
}

// ============================================================================
// Main Verification Function
// ============================================================================

bool VerifySceneStorage(const ufbx_scene* scene, FbxContext& context) {
    std::cout << "\n=== Verifying SceneStorage against ufbx_scene ===" << std::endl;
    
    bool all_passed = true;
    
    if (!VerifyFaces(scene, context.storage)) {
        all_passed = false;
    }
    
    if (!VerifyAllAttributes(scene, context.storage)) {
        all_passed = false;
    }
    
    if (!VerifyNodeTransforms(scene, context.storage)) {
        all_passed = false;
    }
    
    if (!VerifyNodeHierarchy(scene, context.storage)) {
        all_passed = false;
    }
    
    if (!VerifyMeshAssociations(scene, context.storage)) {
        all_passed = false;
    }
    
    std::cout << "=== Verification Complete ===" << std::endl;
    
    return all_passed;
}

// Helper function to export mesh to OBJ format with only positions and tangents
void ExportMeshToOBJ(const std::string& filename, SceneStorage& storage, uint32_t mesh_index) {
    if (mesh_index >= storage.mesh_infos.size()) {
        std::cerr << "Invalid mesh index: " << mesh_index << std::endl;
        return;
    }
    
    MeshInfo& mesh_info = storage.mesh_infos[mesh_index];
    
    std::fstream obj_file;
    obj_file.open(filename, std::ios::out);
    if (!obj_file.is_open()) {
        std::cerr << "Failed to create OBJ file: " << filename << std::endl;
        return;
    }

    objwriter::ObjWriter w(obj_file);
    
    // Find position and normal attributes
    AttributeInfo* position_attrib = nullptr;
    AttributeInfo* normal_attrib = nullptr;
    
    for (uint32_t i = 0; i < mesh_info.attribute_info_count; ++i) {
        AttributeInfo& attrib_info = storage.attrib_infos[mesh_info.attrib_info_start_index + i];
        if (attrib_info.attrib_type == VertexAttribType::Position) {
            position_attrib = &attrib_info;
        } else if (attrib_info.attrib_type == VertexAttribType::Normal) {
            normal_attrib = &attrib_info;
        }
    }
    
    // Write positions to OBJ file
    if (position_attrib) {
        AttributeView position_view = GetAttribView(storage, *position_attrib);
        w.comment("Positions");
        
        for (uint32_t i = 0; i < position_view.data.size() / 3; ++i) {
            
            float x = position_view.data[i * 3 + 0];
            float y = position_view.data[i * 3 + 1];
            float z = position_view.data[i * 3 + 2];
            w.vertex(x , y, z);
        }
    }
    
    // Write normals to OBJ file
    // if (normal_attrib) {
    //     AttributeView normal_view = GetAttribView(storage, *normal_attrib);
    //     obj_file << "# Normals\n";
        
    //     for (uint32_t i = 0; i < normal_view.data.size() / 3; ++i) {
    //         float nx = normal_view.data[i * 3 + 0];
    //         float ny = normal_view.data[i * 3 + 1];
    //         float nz = normal_view.data[i * 3 + 2];
    //         obj_file << "vn " << nx << " " << ny << " " << nz << "\n";
    //     }
    //     obj_file << "\n";
    // }
    
    // Write face indices (convert to OBJ format - OBJ is 1-indexed)
    w.comment("Faces");
    
    if (position_attrib) {
        AttributeView position_view = GetAttribView(storage, *position_attrib);
        FaceView face_view = GetFaceView(storage, mesh_info);
        
        // For triangular faces, write each face
        for (uint32_t face_idx = 0; face_idx < mesh_info.face_count; ++face_idx) {
            Face& face = face_view.faces[face_idx];
            if (face.num_of_indices > 3) 
                std::cout << "Indorrect face count" << std::endl;
            uint32_t v1 = position_view.indices[face.indices_begin + 0] + 1; // OBJ is 1-indexed
            uint32_t v2 = position_view.indices[face.indices_begin + 1] + 1;
            uint32_t v3 = position_view.indices[face.indices_begin + 2] + 1;
            
            w.face(v1, v2, v3);
            obj_file << "f " << v1 << " " << v2 << " " << v3 << "\n";
        }
    }
    
    obj_file.close();
    std::cout << "Exported mesh " << mesh_index << " to " << filename << std::endl;
}

// Test function to verify FBX importer functionality
bool TestFbxImporter(const char* fbx_filename) {
    std::cout << "Testing FBX Importer with file: " << fbx_filename << std::endl;
    
    // Load FBX file
    ufbx_load_opts load_opts = {};
    ufbx_error error = {};
    
    ufbx_scene* scene = ufbx_load_file(fbx_filename, &load_opts, &error);
    if (!scene) {
        std::cerr << "Failed to load FBX file" << std::endl;
        std::cerr << error.description.data << std::endl;
        return false;
    }

    for (size_t i = 0; i < scene->nodes.count; i++) {
        ufbx_node *node = scene->nodes.data[i];
        printf("%s\n", node->name.data);
    }
    
    std::cout << "Loaded FBX scene successfully" << std::endl;
    std::cout << "Found " << scene->nodes.count << " nodes" << std::endl;
    std::cout << "Found " << scene->meshes.count << " meshes" << std::endl;
    
    // Create storage and context
    FbxContext context;
    context.scene = scene;
    
    // Import the scene
    ImportScene(context);
    
    std::cout << "Scene imported successfully" << std::endl;
    
    // Export each mesh to OBJ for verification
    std::cout << "Exporting meshes to OBJ files..." << std::endl;
    //context.storage.mesh_infos.size()
    for (uint32_t i = 0; i < 2; ++i) {
         std::string obj_filename = "mesh_" + std::to_string(i) + ".obj";
         ExportMeshToOBJ(obj_filename, context.storage, i);
    }
    
    // Print node hierarchy information
    // std::cout << "\nNode hierarchy:" << std::endl;
    // for (uint32_t i = 0; i < context.storage.nodes.size(); ++i) {
    //     Node& node = context.storage.nodes[i];
    //     std::cout << "Node " << i << ": parent=" << node.parent 
    //               << ", mesh_index=" << node.mesh_index << std::endl;
    // }
    
    // // Print mesh information
    // std::cout << "\nMesh information:" << std::endl;
    // for (uint32_t i = 0; i < context.storage.mesh_infos.size(); ++i) {
    //     MeshInfo& mesh = context.storage.mesh_infos[i];
    //     std::cout << "Mesh " << i << ": faces=" << mesh.face_count
    //               << ", attributes=" << mesh.attribute_info_count << std::endl;
        
    //     // Print attribute details
    //     for (uint32_t j = 0; j < mesh.attribute_info_count; ++j) {
    //         AttributeInfo& attrib = context.storage.attrib_infos[mesh.attrib_info_start_index + j];
    //         std::cout << "  Attr " << j << ": type=" << static_cast<uint32_t>(attrib.attrib_type)
    //                   << ", indices=" << attrib.index_count
    //                   << ", values=" << attrib.value_count << std::endl;
    //     }
    // }
    
    // Verify SceneStorage values against ufbx_scene
    bool verification_passed = VerifySceneStorage(scene, context);
    
    // Clean up
    ufbx_free_scene(scene);
    
    if (verification_passed) {
        std::cout << "\nTest completed successfully!" << std::endl;
        return true;
    } else {
        std::cout << "\nTest FAILED: Verification found mismatches!" << std::endl;
        return false;
    }
}

} // namespace mesh2py::fbxtest

// Main function for standalone testing
int main(int argc, char* argv[]) {
    // if (argc != 2) {
    //     std::cout << "Usage: " << argv[0] << " <fbx_file>" << std::endl;
    //     return 1;
    // }
    
    const char* fbx_filename = argv[1];
    
    bool success = mesh2py::fbxtest::TestFbxImporter("D:/projects/mesh2usd/build/Debug/kb3d_secretsoftheluminara-native.fbx");
    
    if (success) {
        std::cout << "\nTest PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nTest FAILED!" << std::endl;
        return 1;
    }
}