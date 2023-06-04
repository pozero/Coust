#include "pch.h"

#include "utils/Compiler.h"
#include "render/Mesh.h"

namespace coust {
namespace render {

std::pair<bool, VertexAttrib> to_vertex_attrib(
    std::string_view attribute_name) noexcept {
    if (std::string_view{"POSITION"} == attribute_name) {
        return std::make_pair(true, position);
    } else if (std::string_view{"NORMAL"} == attribute_name) {
        return std::make_pair(true, normal);
    } else if (std::string_view{"TANGENT"} == attribute_name) {
        return std::make_pair(true, tangent);
    } else if (std::string_view{"COLOR_0"} == attribute_name) {
        return std::make_pair(true, color_0);
    } else if (std::string_view{"TEXCOORD_0"} == attribute_name) {
        return std::make_pair(true, texcoord_0);
    } else if (std::string_view{"TEXCOORD_1"} == attribute_name) {
        return std::make_pair(true, texcoord_1);
    } else if (std::string_view{"TEXCOORD_2"} == attribute_name) {
        return std::make_pair(true, texcoord_2);
    } else if (std::string_view{"TEXCOORD_3"} == attribute_name) {
        return std::make_pair(true, texcoord_3);
    }
    return std::make_pair(false, (VertexAttrib) -1);
}

std::string_view to_string_view(VertexAttrib attrib) noexcept {
    switch (attrib) {
        case position:
            return "POSITION";
        case normal:
            return "NORMAL";
        case tangent:
            return "TANGENT";
        case color_0:
            return "COLOR_0";
        case texcoord_0:
            return "TEXCOORD_0";
        case texcoord_1:
            return "TEXCOORD_1";
        case texcoord_2:
            return "TEXCOORD_2";
        case texcoord_3:
            return "TEXCOORD_3";
    }
    ASSUME(0);
}

size_t MeshAggregate::get_primitve_count(MeshAggregate const& ma) {
    size_t ret = 0;
    for (Mesh const& m : ma.meshes) {
        ret += m.primitives.size();
    }
    return ret;
}

}  // namespace render
}  // namespace coust
