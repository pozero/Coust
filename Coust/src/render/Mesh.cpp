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

}  // namespace render
}  // namespace coust
