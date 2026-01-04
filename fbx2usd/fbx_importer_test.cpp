#include "fbx_importer.h"

#include <ufbx.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>

namespace mesh2usd::fbx2usd {

// Helper function to export mesh to OBJ format with only positions and tangents
void ExportMeshToOBJ(const std::string& filename, SceneStorage& storage, uint32_t mesh_index) {
    SceneView scene_view = GetSceneView(storage);
    MeshInfo mesh_info = scene_view.meshes_info[mesh_index];
    MeshView mesh_view = GetMeshView(storage, mesh_info);
    
    std::ofstream obj_file(filename);
    if (!obj_file.is_open()) {
        std::cerr << "Failed to create OBJ file: " << filename << std::endl;
        return;
    }
    
    obj_file << "# Exported from FBX Importer Test\n";
    obj_file << "# Mesh " << mesh_index << " - Positions and Normals only\n\n";
    
    // Extract positions and normals
    std::vector<ufbx_vec3> positions;
    std::vector<ufbx_vec3> normals;
    
    // Find position and normal attributes
    AttributeInfo* position_attrib = nullptr;
    AttributeInfo* normal_attrib = nullptr;
    
    for (uint32_t i = 0; i < mesh_info.attribute_count; ++i) {
        if (mesh_view.attribs_info[i].attrib_type == VertexAttribType::Position) {
            position_attrib = &mesh_view.attribs_info[i];
        } else if (mesh_view.attribs_info[i].attrib_type == VertexAttribType::Normal) {
            normal_attrib = &mesh_view.attribs_info[i];
        }
    }
    
    // Write positions to OBJ file
    if (position_attrib) {
        AttributeView position_view = GetAttribView(storage, mesh_info, *position_attrib);
        obj_file << "# Positions\n";
        
        for (uint32_t i = 0; i < position_view.data.size() / 3; ++i) {
            ufbx_vec3 pos;
            pos.x = position_view.data[i * 3 + 0];
            pos.y = position_view.data[i * 3 + 1];
            pos.z = position_view.data[i * 3 + 2];
            obj_file << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
        }
    }
    
    // Write normals to OBJ file
    if (normal_attrib) {
        AttributeView normal_view = GetAttribView(storage, mesh_info, *normal_attrib);
        obj_file << "# Normals\n";
        
        for (uint32_t i = 0; i < normal_view.data.size() / 3; ++i) {
            ufbx_vec3 normal;
            normal.x = normal_view.data[i * 3 + 0];
            normal.y = normal_view.data[i * 3 + 1];
            normal.z = normal_view.data[i * 3 + 2];
            obj_file << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
        }
        obj_file << "\n";
    }
    
    // Write face indices (convert to OBJ format - OBJ is 1-indexed)
    obj_file << "# Faces\n";
    
    if (position_attrib) {
        AttributeView position_indices = GetAttribView(storage, mesh_info, *position_attrib);
        
        // For triangular faces, write each face
        for (uint32_t face_idx = 0; face_idx < mesh_info.face_count; ++face_idx) {
            uint32_t face_start = mesh_view.face_start_index[face_idx];
            
            // Assuming triangular faces
            if (face_start + 2 < position_indices.indices.size()) {
                uint32_t v1 = position_indices.indices[face_start + 0] + 1; // OBJ is 1-indexed
                uint32_t v2 = position_indices.indices[face_start + 1] + 1;
                uint32_t v3 = position_indices.indices[face_start + 2] + 1;
                
                obj_file << "f " << v1 << " " << v2 << " " << v3 << "\n";
            }
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
    SceneStorage storage;
    storage.scene_info.node_count = scene->nodes.count;
    storage.scene_info.mesh_count = scene->meshes.count;
    
    FbxContext context = {scene, {}, {}, storage};
    
    // Import the scene
    ImportScene(&context);
    
    std::cout << "Scene imported successfully" << std::endl;
    
    // Export each mesh to OBJ for verification
    SceneView scene_view = GetSceneView(storage);
    std::cout << "Exporting meshes to OBJ files..." << std::endl;
    
    for (uint32_t i = 0; i < scene_view.meshes_info.size(); ++i) {
        std::string obj_filename = "mesh_" + std::to_string(i) + ".obj";
        ExportMeshToOBJ(obj_filename, storage, i);
    }
    
    // Print node hierarchy information
    std::cout << "\nNode hierarchy:" << std::endl;
    for (uint32_t i = 0; i < scene_view.nodes.size(); ++i) {
        Node& node = scene_view.nodes[i];
        std::cout << "Node " << i << ": parent=" << node.parent 
                  << ", mesh_index=" << node.mesh_index << std::endl;
    }
    
    // Print mesh information
    std::cout << "\nMesh information:" << std::endl;
    for (uint32_t i = 0; i < scene_view.meshes_info.size(); ++i) {
        MeshInfo& mesh = scene_view.meshes_info[i];
        std::cout << "Mesh " << i << ": faces=" << mesh.face_count 
                  << ", attributes=" << mesh.attribute_count << std::endl;
    }
    
    // Clean up
    ufbx_free_scene(scene);
    
    std::cout << "Test completed successfully!" << std::endl;
    return true;
}

} // namespace mesh2usd::fbx2usd

// Main function for standalone testing
int main(int argc, char* argv[]) {
    // if (argc != 2) {
    //     std::cout << "Usage: " << argv[0] << " <fbx_file>" << std::endl;
    //     return 1;
    // }
    
    const char* fbx_filename = argv[1];
    
    bool success = mesh2usd::fbx2usd::TestFbxImporter("D:/projects/mesh2usd/build/Debug/data/blender_272_cube_7400_binary.fbx");
    
    if (success) {
        std::cout << "\nTest PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nTest FAILED!" << std::endl;
        return 1;
    }
}