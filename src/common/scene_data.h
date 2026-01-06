#pragma once

#include <inttypes.h>
#include <span>
#include <vector>
namespace mesh2py::common {

// Rounds `x` up to a multiple of `a`. `a` must be a power of two.
template <class T>
constexpr T align_up(T x, size_t a) noexcept
{
  return T((x + (T(a) - 1)) & ~T(a - 1));
}

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
struct Node {
    uint32_t parent;
    float transform[16];
    uint32_t mesh_index;
};

struct MeshInfo {
    // Index into the face_infos
    uint32_t face_offset;
    uint32_t face_count;

    // Index into the attrib_infos
    uint32_t attrib_info_start_index;
    uint32_t attribute_info_count;
};

struct AttributeInfo {
    uint32_t index_offset;
    uint32_t value_offset;
    VertexAttribType attrib_type;
    uint32_t index_count;
    uint32_t value_count;
    // This will be 3 float for vector3
    // max_bones float
    // 3 * max_blendshape floats for blendshapes
    uint8_t num_value_per_index;
};

struct Face {
    uint32_t indices_begin;
    uint32_t num_of_indices;
};

struct FaceView {
    std::span<Face> faces;
    // This can contain material id in the future;
};

struct AttributeView {
    std::span<uint32_t> indices;
    std::span<float> data;
};

struct SceneStorage {
    std::vector<Node> nodes;
    std::vector<MeshInfo> mesh_infos;
    std::vector<AttributeInfo> attrib_infos;
    std::vector<uint8_t> data;
};

FaceView GetFaceView(SceneStorage& storage, MeshInfo& mesh_info);
AttributeView GetAttribView(SceneStorage& storage, AttributeInfo& attrib_info);

}

