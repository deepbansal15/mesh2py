#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/ndarray.h>

#include <fbx2py/fbx_importer.h>
#include <common/scene_data.h>

namespace nb = nanobind;
using namespace mesh2py::common;

NB_MODULE(NB_MODULE_NAME, m) {
    m.doc() = "mesh importer module";
    
    // Expose the main import function
    m.def("import_fbx", &ImportFbx,
          nb::arg("path"),
          "Import FBX file and return scene data");
    
    // Expose VertexAttribType enum
    nb::enum_<VertexAttribType>(m, "VertexAttribType")
        .value("Position", VertexAttribType::Position)
        .value("Normal", VertexAttribType::Normal)
        .value("Tangent", VertexAttribType::Tangent)
        .value("BiTangent", VertexAttribType::BiTangent)
        .value("TexCoord", VertexAttribType::TexCoord)
        .value("Color", VertexAttribType::Color)
        .value("Joints", VertexAttribType::Joints)
        .value("Weights", VertexAttribType::Weights)
        .value("Blendshape", VertexAttribType::Blendshape);
    
    using TransformView = nb::ndarray<float, nb::shape<16>, nb::device::cpu, nb::c_contig, nb::numpy>;
    // Expose Node struct
    nb::class_<Node>(m, "Node")
        .def(nb::init<>())
        .def_rw("parent", &Node::parent)
        .def_rw("mesh_index", &Node::mesh_index)
        .def_prop_rw(
            "transform",
            [](Node &self) {
                return TransformView(self.transform);
            },
            [](Node &self,const TransformView& arr) {
                auto v = arr.view();
                for (size_t i = 0; i < 16; ++i)
                    self.transform[i] = v(i);
            },
            nb::rv_policy::reference_internal
        );
    
    // Expose MeshInfo struct
    nb::class_<MeshInfo>(m, "MeshInfo")
        .def(nb::init<>())
        .def_rw("face_offset", &MeshInfo::face_offset)
        .def_rw("face_count", &MeshInfo::face_count)
        .def_rw("attrib_info_start_index", &MeshInfo::attrib_info_start_index)
        .def_rw("attribute_info_count", &MeshInfo::attribute_info_count);
    
    // Expose AttributeInfo struct
    nb::class_<AttributeInfo>(m, "AttributeInfo")
        .def(nb::init<>())
        .def_rw("index_offset", &AttributeInfo::index_offset)
        .def_rw("value_offset", &AttributeInfo::value_offset)
        .def_rw("attrib_type", &AttributeInfo::attrib_type)
        .def_rw("index_count", &AttributeInfo::index_count)
        .def_rw("value_count", &AttributeInfo::value_count)
        .def_rw("num_value_per_index", &AttributeInfo::num_value_per_index);
    
    // Expose Face struct
    nb::class_<Face>(m, "Face")
        .def(nb::init<>())
        .def_rw("indices_begin", &Face::indices_begin)
        .def_rw("num_of_indices", &Face::num_of_indices);
    
    using DataView = nb::ndarray<uint8_t, nb::shape<-1>, nb::device::cpu, nb::c_contig, nb::numpy>;
    // Expose SceneStorage struct
    nb::class_<SceneStorage>(m, "SceneStorage")
        .def(nb::init<>())
        .def_rw("nodes", &SceneStorage::nodes)
        .def_rw("mesh_infos", &SceneStorage::mesh_infos)
        .def_rw("attrib_infos", &SceneStorage::attrib_infos)
        .def_prop_ro(
            "data",
            [](SceneStorage &self) {
                return DataView(self.data.data(),{ self.data.size() });
            },
            nb::rv_policy::reference_internal
        );
}
