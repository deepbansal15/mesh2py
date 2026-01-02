#pragma once

#include <inttypes.h>
#include <span>
#include <vector>

enum class VertexAttribType : uint32_t {
    Position  = 1u << 0,
    Normal    = 1u << 1,
    Tangent   = 1u << 2,
    BiTangent = 1u << 3,
    TexCoord = 1u << 4,
    Color    = 1u << 5,
    Joints   = 1u << 6,
    Weights  = 1u << 7,
    Blendshape = 1u << 8
};

// Only tirangle meshes are supported

struct Node {
    uint32_t parent;
    float transform[16];
    uint32_t mesh_index;
};

struct SceneInfo {
    uint32_t node_offset;
    uint32_t node_count;

    uint32_t mesh_offset;
    uint32_t mesh_count;
};

struct MeshInfo {
    uint32_t face_offset;
    uint32_t face_count;
    uint32_t attrib_offset;
    uint32_t attribute_count;
};

struct AttributeInfo {
    uint32_t data_offset;
    VertexAttribType attrib_type;
    uint32_t index_count;
    uint32_t data_count;
    // This will be 3 float for vector3
    // max_bones float
    // 3 * max_blendshape floats for blendshapes
    uint8_t num_value_per_index;
};

struct AttributeView {
    std::span<uint32_t> indices;
    std::span<float> data;
};  

struct MeshView {
    std::span<uint32_t> face_start_index;
    std::span<AttributeInfo> attribs_info;
};

struct SceneView {
    std::span<Node> nodes;
    std::span<MeshInfo> meshes_info;
};

struct SceneStorage {
    SceneInfo scene_info;
    std::vector<uint8_t> data;
};

SceneView GetSceneView(SceneStorage& storage);

void CalculateMeshOffsets(SceneView& scene_view, SceneStorage& storage);

MeshView GetMeshView(SceneStorage& storage, MeshInfo mesh_info);

void CalculateAttributeOffsets(MeshView& mesh_view, SceneStorage& storage);

AttributeView GetAttribView(SceneStorage& storage, MeshInfo mesh_info, AttributeInfo& attrib_info);




