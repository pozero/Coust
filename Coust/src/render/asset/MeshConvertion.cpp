#include "pch.h"

#include "utils/math/NormalizedUInteger.h"
#include "utils/PtrMath.h"
#include "utils/Assert.h"
#include "utils/Compiler.h"
#include "render/asset/MeshConvertion.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "tiny_gltf.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
CLANG_DISABLE_WARNING("-Wcast-align")
// return the start index of current attribute.
inline size_t copy_vertex_data_from_gltf_buffer(tinygltf::Model const& model,
    size_t accessor_idx, memory::vector<float, DefaultAlloc>& out_attrib_data,
    // we stipulate that the type color must be vec4. so if the rgb color is
    // provided, we need to suffix the data with 1.0f.
    bool is_color) noexcept {
    auto const& gltf_accessor = model.accessors[accessor_idx];
    auto const& gltf_bufview =
        model.bufferViews[(size_t) gltf_accessor.bufferView];
    const uint8_t* const gltf_buf_data_begin = ptr_math::add(
        ptr_math::add(model.buffers[(size_t) gltf_bufview.buffer].data.data(),
            gltf_bufview.byteOffset),
        gltf_accessor.byteOffset);
    // according to glTF 2.0 spec, for the attributes we selected, there are
    // three possible component type:
    // - float
    // - unsigned byte normalized
    // - unsigned short normalized
    bool const is_float =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT;
    bool const is_ubyte =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
        gltf_accessor.normalized;
    bool const is_ushort =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
        gltf_accessor.normalized;
    COUST_PANIC_IF_NOT(is_float || is_ubyte || is_ushort,
        "Unknown component type: {}", gltf_accessor.componentType);

    size_t const element_component_count =
        (size_t) tinygltf::GetNumComponentsInType(
            (uint32_t) gltf_accessor.type);
    bool const is_rgb_color = is_color && element_component_count == 3;

    size_t const element_size = (size_t) tinygltf::GetComponentSizeInBytes(
                                    (uint32_t) gltf_accessor.componentType) *
                                element_component_count;
    size_t const float_count_to_reserve =
        out_attrib_data.size() +
        (is_rgb_color ? gltf_accessor.count * 4 :
                        gltf_accessor.count * element_component_count);
    out_attrib_data.reserve(float_count_to_reserve);
    size_t const ret = out_attrib_data.size();

    size_t const stride_size =
        gltf_bufview.byteStride == 0 ? element_size : gltf_bufview.byteStride;
    const void* begin = gltf_buf_data_begin;
    const void* end = ptr_math::add(begin, element_size);
    for (size_t i = 0; i < gltf_accessor.count; ++i,
                begin = ptr_math::add(begin, stride_size),
                end = ptr_math::add(begin, element_size)) {
        if (is_float) {
            std::copy((const float*) begin, (const float*) end,
                std::back_inserter(out_attrib_data));
        } else if (is_ubyte) {
            std::transform((const uint8_t*) begin, (const uint8_t*) end,
                std::back_inserter(out_attrib_data), ubyte_to_float);
        } else /* is_ushort */ {
            std::transform((const uint16_t*) begin, (const uint16_t*) end,
                std::back_inserter(out_attrib_data), ushort_to_float);
        }
        if (is_rgb_color) {
            out_attrib_data.push_back(1.0f);
        }
    }

    COUST_ASSERT(
        (out_attrib_data.size() - ret) % element_component_count == 0, "");
    COUST_ASSERT((out_attrib_data.size() - ret) / element_component_count ==
                     gltf_accessor.count,
        "");

    return ret;
}
WARNING_POP

inline std::pair<uint32_t, uint32_t> copy_index_data_from_gltf_buffer(
    tinygltf::Model const& model, size_t accessor_idx,
    memory::vector<uint32_t, DefaultAlloc>& out_index_data) noexcept {
    auto const& gltf_accessor = model.accessors[accessor_idx];
    auto const& gltf_bufview =
        model.bufferViews[(size_t) gltf_accessor.bufferView];
    bool const is_sbyte =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE;
    bool const is_ubyte =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    bool const is_sshort =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT;
    bool const is_ushort =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    bool const is_sint =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT;
    bool const is_uint =
        gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    COUST_PANIC_IF_NOT(
        is_sbyte || is_ubyte || is_sshort || is_ushort || is_sint || is_uint,
        "Unknown component type: {}", gltf_accessor.componentType);
    size_t const ret = out_index_data.size();
    COUST_PANIC_IF_NOT(gltf_bufview.byteStride == 0,
        "Converter assumes the index data should be tightly stored");
    size_t const element_component_count =
        (size_t) tinygltf::GetNumComponentsInType(
            (uint32_t) gltf_accessor.type);
    size_t const element_size = (size_t) tinygltf::GetComponentSizeInBytes(
                                    (uint32_t) gltf_accessor.componentType) *
                                element_component_count;
    const void* const gltf_buf_data_begin = ptr_math::add(
        ptr_math::add(model.buffers[(size_t) gltf_bufview.buffer].data.data(),
            gltf_bufview.byteOffset),
        gltf_accessor.byteOffset);
    const void* const gltf_buf_data_end =
        ptr_math::add(gltf_buf_data_begin, element_size * gltf_accessor.count);
    out_index_data.reserve(
        out_index_data.size() + gltf_accessor.count * element_component_count);
    if (is_sbyte) {
        using source_type = int8_t;
        std::copy((const source_type*) gltf_buf_data_begin,
            (const source_type*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    } else if (is_ubyte) {
        using source_type = uint8_t;
        std::copy((const source_type*) gltf_buf_data_begin,
            (const source_type*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    } else if (is_sshort) {
        using source_type = int16_t;
        std::copy((const source_type*) gltf_buf_data_begin,
            (const source_type*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    } else if (is_ushort) {
        using source_type = uint16_t;
        std::copy((const source_type*) gltf_buf_data_begin,
            (const source_type*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    } else if (is_sint) {
        using source_type = int32_t;
        std::copy((const source_type*) gltf_buf_data_begin,
            (const source_type*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    } else /* is_uint */ {
        std::copy((const uint32_t*) gltf_buf_data_begin,
            (const uint32_t*) gltf_buf_data_end,
            std::back_inserter(out_index_data));
    }
    COUST_ASSERT((out_index_data.size() - ret) == gltf_accessor.count, "");
    COUST_PANIC_IF(ret > std::numeric_limits<uint32_t>::max(), "");
    return std::make_pair((uint32_t) ret, (uint32_t) gltf_accessor.count);
}

}  // namespace detail

MeshAggregate process_gltf(std::filesystem::path path) noexcept {
    tinygltf::Model model{};
    tinygltf::TinyGLTF loader{};
    std::string tinygltf_err{}, tinygltf_warn{};
    bool tinygltf_success = loader.LoadASCIIFromFile(
        &model, &tinygltf_err, &tinygltf_warn, path.string());
    COUST_PANIC_IF_NOT(tinygltf_success, "tinygltf: ERR {}, WARN {}",
        tinygltf_err, tinygltf_warn);

    MeshAggregate mesh_aggregate{};
    std::array<memory::vector<float, DefaultAlloc>, 8> all_attrib_data{
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
        memory::vector<float, DefaultAlloc>{get_default_alloc()},
    };

    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    auto const extract_vertex_attrib_data =
        [&model, &all_attrib_data, &mesh_aggregate](
            tinygltf::Primitive const& gltf_primitive) {
            Mesh::Primitive primitive{};
            COUST_PANIC_IF(gltf_primitive.indices < 0,
                "Converter only support meshes with indices");
            std::tie(primitive.index_offset, primitive.index_count) =
                detail::copy_index_data_from_gltf_buffer(model,
                    (size_t) gltf_primitive.indices,
                    mesh_aggregate.index_buffer);
            for (auto const& [gltf_attrib_name, gltf_accessor_idx] :
                gltf_primitive.attributes) {
                auto [is_needed, attrib_idx] =
                    to_vertex_attrib(gltf_attrib_name);
                if (!is_needed)
                    continue;
                auto& destination = all_attrib_data[attrib_idx];
                bool const is_color = attrib_idx == VertexAttrib::color_0;
                size_t cur_attrib_idx =
                    detail::copy_vertex_data_from_gltf_buffer(model,
                        (size_t) gltf_accessor_idx, destination, is_color);
                primitive.attrib_offset[attrib_idx] = (uint32_t) cur_attrib_idx;
                if (!is_color && attrib_idx == VertexAttrib::position) {
                    static_assert(sizeof(glm::vec3) == 3 * sizeof(float));
                    const glm::vec3* const begin =
                        (glm::vec3*) &destination[cur_attrib_idx];
                    const glm::vec3* const end =
                        ((glm::vec3*) &destination.back()) + 1;
                    std::span<glm::vec3 const> const points{begin, end};
                    primitive.bounding_box = BoundingBox{points};
                }
            }
            // TODO: ignore material info for now
            return primitive;
        };
    WARNING_POP

    mesh_aggregate.meshes.reserve(model.meshes.size());
    for (auto const& gltf_mesh : model.meshes) {
        Mesh mesh{};
        mesh.primitives.reserve(gltf_mesh.primitives.size());
        for (auto const& gltf_primitive : gltf_mesh.primitives) {
            mesh.primitives.push_back(
                extract_vertex_attrib_data(gltf_primitive));
        }
        mesh_aggregate.meshes.push_back(std::move(mesh));
    }

    // pack different attributes data together
    size_t float_count_offset = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        COUST_PANIC_IF(
            float_count_offset > std::numeric_limits<uint32_t>::max(), "");
        mesh_aggregate.attrib_bytes_offset[i] =
            float_count_offset * sizeof(float);
        if (!all_attrib_data[i].empty()) {
            mesh_aggregate.valid_attrib_mask |= (1 << i);
            for (auto& mesh : mesh_aggregate.meshes) {
                for (auto& primitive : mesh.primitives) {
                    primitive.attrib_offset[i] += (uint32_t) float_count_offset;
                }
            }
            float_count_offset += all_attrib_data[i].size();
        }
    }
    mesh_aggregate.vertex_buffer.reserve(float_count_offset);
    for (uint32_t i = 0; i < all_attrib_data.size(); ++i) {
        if (!all_attrib_data[i].empty()) {
            std::copy(all_attrib_data[i].begin(), all_attrib_data[i].end(),
                std::back_inserter(mesh_aggregate.vertex_buffer));
        }
    }

    // the glTF file doesn't define scenes
    if (model.defaultScene == -1) {
        mesh_aggregate.nodes.resize(mesh_aggregate.meshes.size());
        for (size_t i = 0; i < mesh_aggregate.meshes.size(); ++i) {
            mesh_aggregate.nodes[i].mesh_idx = (uint32_t) i;
        }
    } else {
        // fill in mesh index
        auto const& gltf_default_scene =
            model.scenes[(size_t) model.defaultScene];
        mesh_aggregate.nodes.reserve(model.nodes.size());
        std::ranges::transform(model.nodes,
            std::back_inserter(mesh_aggregate.nodes),
            [](tinygltf::Node const& node) {
                Node ret{};
                ret.mesh_idx = (uint32_t) node.mesh;
                return ret;
            });

        // generate transformation tree
        for (uint32_t i = 0; i < model.nodes.size(); ++i) {
            auto const& node = model.nodes[i];
            glm::mat4 local_transformation{1.0f};
            if (node.matrix.size() == 16) {
                local_transformation = glm::mat4{
                    (float) node.matrix[0],
                    (float) node.matrix[1],
                    (float) node.matrix[2],
                    (float) node.matrix[3],
                    (float) node.matrix[4],
                    (float) node.matrix[5],
                    (float) node.matrix[6],
                    (float) node.matrix[7],
                    (float) node.matrix[8],
                    (float) node.matrix[9],
                    (float) node.matrix[10],
                    (float) node.matrix[11],
                    (float) node.matrix[12],
                    (float) node.matrix[13],
                    (float) node.matrix[14],
                    (float) node.matrix[15],
                };
            } else {
                if (node.translation.size() == 3) {
                    glm::vec3 t{(float) node.translation[0],
                        (float) node.translation[1],
                        (float) node.translation[2]};
                    local_transformation =
                        glm::translate(local_transformation, t);
                }
                if (node.rotation.size() == 4) {
                    glm::quat q{(float) node.rotation[0],
                        (float) node.rotation[1], (float) node.rotation[2],
                        (float) node.rotation[3]};
                    local_transformation *= glm::toMat4(q);
                }
                if (node.scale.size() == 3) {
                    glm::vec3 s{(float) node.scale[0], (float) node.scale[1],
                        (float) node.scale[2]};
                    local_transformation = glm::scale(local_transformation, s);
                }
            }
            mesh_aggregate.transformations.push_back(local_transformation);
        }
        memory::deque<uint32_t, DefaultAlloc> node_stack{get_default_alloc()};
        {
            memory::vector<bool, DefaultAlloc> is_root_node{
                gltf_default_scene.nodes.size(), true, get_default_alloc()};
            for (uint32_t i = 0; i < gltf_default_scene.nodes.size(); ++i) {
                for (int c : model.nodes[(uint32_t) gltf_default_scene.nodes[i]]
                                 .children) {
                    is_root_node[(uint32_t) c] = false;
                }
            }
            for (uint32_t i = 0; i < gltf_default_scene.nodes.size(); ++i) {
                if (is_root_node[i]) {
                    node_stack.push_front(
                        (uint32_t) gltf_default_scene.nodes[i]);
                }
            }
        }
        memory::vector<bool, DefaultAlloc> is_recorded{
            model.nodes.size(), false, get_default_alloc()};
        while (!node_stack.empty()) {
            COUST_ASSERT(node_stack.size() <= is_recorded.size(), "");
            uint32_t const node_idx = node_stack.front();
            if (is_recorded[node_idx]) {
                node_stack.pop_front();
                continue;
            }
            auto& global_transform_indices =
                mesh_aggregate.nodes[node_idx].transformation_indices;
            for (uint32_t n : node_stack) {
                global_transform_indices.push_back(n);
            }
            for (int c : model.nodes[node_idx].children) {
                node_stack.push_front((uint32_t) c);
            }
            is_recorded[node_idx] = true;
        }
    }
    return mesh_aggregate;
}

}  // namespace render
}  // namespace coust
