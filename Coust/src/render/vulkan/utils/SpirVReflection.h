#pragma once

#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"

namespace coust {
namespace render {

enum class ShaderResourceType {
    input,                    // stage_inputs
    input_attachment,         // subpass_inputs
    output,                   // stage_outputs
    image,                    // separate_images
    sampler,                  // separate_samplers
    image_sampler,            // sampled_images
    image_storage,            // storage_images
    uniform_buffer,           // uniform_buffers
    storage_buffer,           // storage_buffers
    push_constant,            // push_constant_buffers
    specialization_constant,  // `get_specialization_constants()`
};

enum class ShaderResourceBaseType {
    boolean,
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    half_float,
    single_float,
    double_float,
    structure,
    unknown,
};

// These shader resource update mode can't be configured in shader module class,
// their modification is deferred until the cosntruction of pipeline layout
enum class ShaderResourceUpdateMode {
    static_update,
    dynamic_update,
    update_after_bind,
};

// TODO: didn't find an easy way to serialize shder resource member yet, disable
// for now

// struct ShaderResourceMember {
//     memory::string<DefaultAlloc> name{get_default_alloc()};
//     ShaderResourceBaseType base_type;
//     size_t size;
//     uint32_t offset;
//     uint32_t vec_size;
//     uint32_t columns;
//     uint32_t array_size;
//     ShaderResourceMember *next_member = nullptr;
//     ShaderResourceMember *members = nullptr;
// };

struct ShaderResource {
    // Fields used by all types of shader resource
    memory::string<DefaultAlloc> name{get_default_alloc()};
    unsigned int vk_shader_stage = 0;
    ShaderResourceType type;

    // Fields of decoration
    ShaderResourceUpdateMode update_mode;
    unsigned int vk_access = 0;
    // ShaderResourceMember *members = nullptr;
    ShaderResourceBaseType base_type;
    uint32_t set;
    uint32_t binding;
    uint32_t location;
    uint32_t input_attachment_idx;
    uint32_t vec_size;
    uint32_t columns;
    uint32_t array_size;
    uint32_t offset;
    uint32_t size;
    uint32_t constant_id;
};

namespace detail {

auto spirv_reflection(std::span<const uint32_t> byte_code, int vk_shader_stage,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const &desired_runtime_size) noexcept
    -> memory::vector<ShaderResource, DefaultAlloc>;

}  // namespace detail
}  // namespace render
}  // namespace coust
