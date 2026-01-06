#include "scene_data.h"

namespace mesh2py::common {

FaceView GetFaceView(SceneStorage& storage, MeshInfo& mesh_info) {
    size_t base_addr = (size_t)storage.data.data();
    size_t ptr = base_addr + mesh_info.face_offset;
    FaceView ret;
    ret.faces = std::span<Face>((Face*)ptr, mesh_info.face_count);
    return ret;
}

AttributeView GetAttribView(SceneStorage& storage, AttributeInfo& attrib_info) {
    size_t base_addr = (size_t)storage.data.data();
    size_t index_ptr = base_addr + attrib_info.index_offset;
    size_t value_ptr = base_addr + attrib_info.value_offset;

    AttributeView ret;
    ret.indices = std::span<uint32_t>((uint32_t*)index_ptr, attrib_info.index_count);
    ret.data = std::span<float>((float*)value_ptr, attrib_info.value_count * attrib_info.num_value_per_index);
    return ret;
}

}