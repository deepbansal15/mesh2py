#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/span.h>

#include <fbx2py/fbx_importer.h>
#include <common/scene_data.h>

namespace nb = nanobind;
using namespace mesh2py::common;

NB_MODULE(fbx_importer, m) {
    m.doc() = "FBX importer module using nanobind";
    
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
    
    // Expose Node struct
    nb::class_<Node>(m, "Node")
        .def(nb::init<>())
        .def_property("parent", &Node::parent, &Node::parent)
        .def_property("mesh_index", &Node::mesh_index, &Node::mesh_index)
        .def_property_readonly("transform", [](Node& self) {
            return nb::cast(std::span<float>(self.transform, 16));
        });
    
    // Expose MeshInfo struct
    nb::class_<MeshInfo>(m, "MeshInfo")
        .def(nb::init<>())
        .def_property("face_offset", &MeshInfo::face_offset, &MeshInfo::face_offset)
        .def_property("face_count", &MeshInfo::face_count, &MeshInfo::face_count)
        .def_property("attrib_info_start_index", &MeshInfo::attrib_info_start_index, &MeshInfo::attrib_info_start_index)
        .def_property("attribute_info_count", &MeshInfo::attribute_info_count, &MeshInfo::attribute_info_count);
    
    // Expose AttributeInfo struct
    nb::class_<AttributeInfo>(m, "AttributeInfo")
        .def(nb::init<>())
        .def_property("index_offset", &AttributeInfo::index_offset, &AttributeInfo::index_offset)
        .def_property("value_offset", &AttributeInfo::value_offset, &AttributeInfo::value_offset)
        .def_property("attrib_type", &AttributeInfo::attrib_type, &AttributeInfo::attrib_type)
        .def_property("index_count", &AttributeInfo::index_count, &AttributeInfo::index_count)
        .def_property("value_count", &AttributeInfo::value_count, &AttributeInfo::value_count)
        .def_property("num_value_per_index", &AttributeInfo::num_value_per_index, &AttributeInfo::num_value_per_index);
    
    // Expose Face struct
    nb::class_<Face>(m, "Face")
        .def(nb::init<>())
        .def_property("indices_begin", &Face::indices_begin, &Face::indices_begin)
        .def_property("num_of_indices", &Face::num_of_indices, &Face::num_of_indices);
    
    // Expose FaceView struct
    nb::class_<FaceView>(m, "FaceView")
        .def(nb::init<>())
        .def_property_readonly("faces", [](FaceView& self) {
            return nb::cast(self.faces);
        });
    
    // Expose AttributeView struct
    nb::class_<AttributeView>(m, "AttributeView")
        .def(nb::init<>())
        .def_property_readonly("indices", [](AttributeView& self) {
            return nb::cast(self.indices);
        })
        .def_property_readonly("data", [](AttributeView& self) {
            return nb::cast(self.data);
        });
    
    // Expose SceneStorage struct
    nb::class_<SceneStorage>(m, "SceneStorage")
        .def(nb::init<>())
        .def_property_readonly("nodes", [](SceneStorage& self) {
            return nb::cast(self.nodes);
        })
        .def_property_readonly("mesh_infos", [](SceneStorage& self) {
            return nb::cast(self.mesh_infos);
        })
        .def_property_readonly("attrib_infos", [](SceneStorage& self) {
            return nb::cast(self.attrib_infos);
        })
        .def_property_readonly("data", [](SceneStorage& self) {
            return nb::bytes(reinterpret_cast<const char*>(self.data.data()), self.data.size());
        });
}
