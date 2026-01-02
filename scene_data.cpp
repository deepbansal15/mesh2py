#include "scene_data.h"

/*-------------------------------------------------------------------------------------------------
# Function `align_up<integral>(x, a)`
Rounds `x` up to a multiple of `a`. `a` must be a power of two.
-------------------------------------------------------------------------------------------------*/
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
  return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

SceneView GetSceneView(SceneStorage& storage) {
    size_t startAddress = (size_t)storage.data.data();
    
    // Calculate required size for node data with alignment
    size_t node_offset = align_up(storage.data.size(), 16);
    size_t node_size = storage.scene_info.node_count * sizeof(Node);
    size_t required_size_after_node = node_offset + node_size;
    
    // Calculate required size for mesh info with alignment
    size_t mesh_offset = align_up(required_size_after_node, 16);
    size_t mesh_size = storage.scene_info.mesh_count * sizeof(MeshInfo);
    size_t total_required_size = mesh_offset + mesh_size;
    
    // Only resize if current size is insufficient
    if (storage.data.size() < total_required_size) {
        storage.data.resize(total_required_size);
    }
    
    // Update offsets in scene_info
    storage.scene_info.node_offset = node_offset;
    storage.scene_info.mesh_offset = mesh_offset;
    
    SceneView ret;
    ret.nodes = std::span<Node>((Node*)(startAddress + node_offset), storage.scene_info.node_count);
    ret.meshes_info = std::span<MeshInfo>((MeshInfo*)(startAddress + mesh_offset), storage.scene_info.mesh_count);
    
    return ret;
}

// Helper function to pre-calculate all mesh offsets efficiently
void CalculateMeshOffsets(SceneView& scene_view, SceneStorage& storage) {
    // Base address where mesh data starts (after mesh_info array)
    size_t base_address = storage.scene_info.mesh_offset + (storage.scene_info.mesh_count * sizeof(MeshInfo));
    
    // Calculate offsets for all meshes in one pass using the SceneView
    size_t current_offset = 0;
    for (size_t i = 0; i < scene_view.meshes_info.size(); ++i) {
        MeshInfo& mesh = scene_view.meshes_info[i];
        
        // Calculate the aligned address for face data
        size_t face_address = align_up(base_address + current_offset, 16);
        mesh.face_offset = static_cast<uint32_t>(face_address);
        
        // Calculate aligned address for attribute data
        size_t face_data_size = mesh.face_count * sizeof(uint32_t);
        size_t attrib_address = align_up(face_address + face_data_size, 16);
        mesh.attrib_offset = static_cast<uint32_t>(attrib_address);
        
        // Update current offset for next mesh
        size_t attrib_data_size = mesh.attribute_count * sizeof(AttributeInfo);
        current_offset = mesh.attrib_offset + attrib_data_size;
    }
    
    // Resize data vector if necessary to accommodate all mesh data
    size_t total_required_size = base_address + current_offset;
    if (storage.data.size() < total_required_size) {
        storage.data.resize(total_required_size);
    }
}

MeshView GetMeshView(SceneStorage& storage, MeshInfo mesh_info) {
    // Get the pre-calculated offsets directly from mesh_info (they're absolute addresses)
    size_t face_offset = mesh_info.face_offset;
    size_t attrib_offset = mesh_info.attrib_offset;
    
    MeshView ret;
    
    // Create spans for the mesh data
    // face_start_index points to the face indices for this mesh
    ret.face_start_index = std::span<uint32_t>((uint32_t*)face_offset, mesh_info.face_count);
    
    // attribs_info contains information about the mesh's attributes
    ret.attribs_info = std::span<AttributeInfo>((AttributeInfo*)attrib_offset, mesh_info.attribute_count);
    
    return ret;
}

// Helper function to calculate attribute offsets for a mesh
// Helper function to calculate attribute offsets for a mesh
void CalculateAttributeOffsets(MeshView& mesh_view, SceneStorage& storage) {
    // Base address where attribute data starts for this mesh (after AttributeInfo array)
    size_t base_address = (size_t)mesh_view.attribs_info.data() + (mesh_view.attribs_info.size() * sizeof(AttributeInfo));
    
    // Calculate offsets for all attributes in one pass using the MeshView
    size_t current_offset = 0;
    for (size_t i = 0; i < mesh_view.attribs_info.size(); ++i) {
        AttributeInfo& attrib = mesh_view.attribs_info[i];
        
        // Calculate the aligned address for attribute data
        size_t attrib_address = align_up(base_address + current_offset, 16);
        attrib.data_offset = static_cast<uint32_t>(attrib_address);
        
        // Update current offset for next attribute
        size_t indices_size = attrib.index_count * sizeof(uint32_t);
        size_t data_size = attrib.data_count * attrib.num_value_per_index * sizeof(float);
        current_offset = attrib.data_offset + indices_size + data_size;
    }
    
    // Resize data vector if necessary to accommodate all attribute data
    size_t total_required_size = base_address + current_offset;
    if (storage.data.size() < total_required_size) {
        storage.data.resize(total_required_size);
    }
}

AttributeView GetAttribView(SceneStorage& storage, MeshInfo mesh_info, AttributeInfo& attrib_info) {
    // Get the pre-calculated data offset directly from attrib_info
    size_t data_offset = attrib_info.data_offset;
    
    AttributeView ret;
    
    // Create spans for the attribute data
    // indices point to the index data for this attribute
    ret.indices = std::span<uint32_t>((uint32_t*)data_offset, attrib_info.index_count);
    
    // data contains the actual attribute values
    size_t values_offset = data_offset + (attrib_info.index_count * sizeof(uint32_t));
    ret.data = std::span<float>((float*)values_offset, attrib_info.data_count * attrib_info.num_value_per_index);
    
    return ret;
}