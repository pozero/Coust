#include "pch.h"

#include "render/vulkan/utils/SpirVReflection.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch")
constexpr ShaderResourceBaseType spirv_type_to_shader_resource_base_type(
    spirv_cross::SPIRType::BaseType base_type) noexcept {
    switch (base_type) {
        case spirv_cross::SPIRType::BaseType::Boolean:
            return ShaderResourceBaseType::boolean;
        case spirv_cross::SPIRType::BaseType::SByte:
            return ShaderResourceBaseType::int8;
        case spirv_cross::SPIRType::BaseType::UByte:
            return ShaderResourceBaseType::uint8;
        case spirv_cross::SPIRType::BaseType::Short:
            return ShaderResourceBaseType::int16;
        case spirv_cross::SPIRType::BaseType::UShort:
            return ShaderResourceBaseType::uint16;
        case spirv_cross::SPIRType::BaseType::Int:
            return ShaderResourceBaseType::int32;
        case spirv_cross::SPIRType::BaseType::UInt:
            return ShaderResourceBaseType::uint32;
        case spirv_cross::SPIRType::BaseType::Int64:
            return ShaderResourceBaseType::int64;
        case spirv_cross::SPIRType::BaseType::UInt64:
            return ShaderResourceBaseType::uint64;
        case spirv_cross::SPIRType::BaseType::Half:
            return ShaderResourceBaseType::half_float;
        case spirv_cross::SPIRType::BaseType::Float:
            return ShaderResourceBaseType::single_float;
        case spirv_cross::SPIRType::BaseType::Double:
            return ShaderResourceBaseType::double_float;
        case spirv_cross::SPIRType::BaseType::Struct:
            return ShaderResourceBaseType::structure;
    }
    return ShaderResourceBaseType::unknown;
}
WARNING_POP

constexpr uint32_t get_shader_resource_base_type_size(
    ShaderResourceBaseType baseType) noexcept {
    switch (baseType) {
        case ShaderResourceBaseType::int8:
        case ShaderResourceBaseType::uint8:
            return 1;
        case ShaderResourceBaseType::int16:
        case ShaderResourceBaseType::uint16:
        case ShaderResourceBaseType::half_float:
            return 2;
        case ShaderResourceBaseType::boolean:
        case ShaderResourceBaseType::int32:
        case ShaderResourceBaseType::uint32:
        case ShaderResourceBaseType::single_float:
            return 4;
        case ShaderResourceBaseType::int64:
        case ShaderResourceBaseType::uint64:
        case ShaderResourceBaseType::double_float:
            return 8;
        case ShaderResourceBaseType::structure:
        case ShaderResourceBaseType::unknown:
            return 0;
    }
}

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch")
constexpr uint32_t get_spirv_base_type_size(
    spirv_cross::SPIRType::BaseType base_type) noexcept {
    switch (base_type) {
        case spirv_cross::SPIRType::BaseType::SByte:
        case spirv_cross::SPIRType::BaseType::UByte:
            return 1;
        case spirv_cross::SPIRType::BaseType::Short:
        case spirv_cross::SPIRType::BaseType::UShort:
        case spirv_cross::SPIRType::BaseType::Half:
            return 2;
        case spirv_cross::SPIRType::BaseType::Boolean:
        case spirv_cross::SPIRType::BaseType::Int:
        case spirv_cross::SPIRType::BaseType::UInt:
        case spirv_cross::SPIRType::BaseType::Float:
            return 4;
        case spirv_cross::SPIRType::BaseType::Int64:
        case spirv_cross::SPIRType::BaseType::UInt64:
        case spirv_cross::SPIRType::BaseType::Double:
            return 8;
    }
    return 0;
}
WARNING_POP

FORCE_INLINE static spirv_cross::SPIRType const& read_spirv_type(
    spirv_cross::CompilerGLSL const& complier,
    spirv_cross::Resource const& spirv_resource) noexcept {
    return complier.get_type_from_variable(spirv_resource.id);
}

FORCE_INLINE static void read_shader_resource_base_type(
    spirv_cross::CompilerGLSL const& complier,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    out_shader_resource.base_type = spirv_type_to_shader_resource_base_type(
        read_spirv_type(complier, spirv_resource).basetype);
}

FORCE_INLINE static void read_shader_resource_decoration_location(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    out_shader_resource.location =
        compiler.get_decoration(spirv_resource.id, spv::DecorationLocation);
}

FORCE_INLINE static void read_shader_resource_decoration_descriptor_set(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    out_shader_resource.set = compiler.get_decoration(
        spirv_resource.id, spv::DecorationDescriptorSet);
}

FORCE_INLINE static void read_shader_resource_decoration_binding(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    out_shader_resource.binding =
        compiler.get_decoration(spirv_resource.id, spv::DecorationBinding);
}

FORCE_INLINE static void read_shader_resource_decoration_attachment_idx(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    out_shader_resource.input_attachment_idx = compiler.get_decoration(
        spirv_resource.id, spv::DecorationInputAttachmentIndex);
}

FORCE_INLINE static void read_shader_resource_decoration_nonreadable(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    // Note: DecorationNonReadable / DecorationNonWritable should be obtained
    // through `get_buffer_block_flags` instead of `get_decoration`
    auto flag = compiler.get_buffer_block_flags(spirv_resource.id);
    if (flag.get(spv::DecorationNonReadable))
        out_shader_resource.vk_access &=
            (unsigned int) ~VK_ACCESS_SHADER_READ_BIT;
}

FORCE_INLINE static void read_shader_resource_decoration_nonwritable(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    // Note: DecorationNonReadable / DecorationNonWritable should be obtained
    // through `get_buffer_block_flags` instead of `get_decoration`
    auto flag = compiler.get_buffer_block_flags(spirv_resource.id);
    if (flag.get(spv::DecorationNonWritable))
        out_shader_resource.vk_access &=
            (unsigned int) ~VK_ACCESS_SHADER_WRITE_BIT;
}

FORCE_INLINE static void read_shader_resource_vec_size(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    auto const& spirv_type = read_spirv_type(compiler, spirv_resource);
    out_shader_resource.vec_size = spirv_type.vecsize;
    out_shader_resource.columns = spirv_type.columns;
}

FORCE_INLINE static void read_shader_resource_array_size(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    ShaderResource& out_shader_resource) noexcept {
    auto const& spirv_type = read_spirv_type(compiler, spirv_resource);
    out_shader_resource.array_size =
        spirv_type.array.size() > 0 ? spirv_type.array[0] : 1;
}

FORCE_INLINE static void read_shader_resource_size(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::Resource const& spirv_resource,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const& desired_runtime_size,
    ShaderResource& out_shader_resource) noexcept {
    auto const& spirv_type = read_spirv_type(compiler, spirv_resource);
    size_t array_size = 0;
    if (desired_runtime_size.contains(out_shader_resource.name)) {
        array_size = desired_runtime_size.at(out_shader_resource.name);
    }
    size_t actual_size =
        compiler.get_declared_struct_size_runtime_array(spirv_type, array_size);
    COUST_PANIC_IF(actual_size > std::numeric_limits<uint32_t>::max(),
        "{} is too big to be converted to uint32_t", actual_size);
    out_shader_resource.size = (uint32_t) actual_size;
}

// static ShaderResourceMember* read_shader_resource_members(
//     spirv_cross::CompilerGLSL const& compiler,
//     spirv_cross::SPIRType const& spirv_type) noexcept {
//     ShaderResourceMember* first_member = nullptr;
//     ShaderResourceMember* pre_member = nullptr;
//     for (size_t i = 0u; i < spirv_type.member_types.size(); ++i) {
//         const auto& memType = compiler.get_type(spirv_type.member_types[i]);
//         ShaderResourceBaseType baseType =
//             spirv_type_to_shader_resource_base_type(memType.basetype);
//         if (baseType == ShaderResourceBaseType::unknown)
//             continue;
//         ShaderResourceMember* mem =
//             get_default_alloc().construct<ShaderResourceMember>();
//         mem->name = compiler.get_member_name(spirv_type.self, uint32_t(i));
//         mem->base_type = baseType;
//         mem->offset =
//             compiler.type_struct_member_offset(spirv_type, uint32_t(i));
//         mem->size =
//             compiler.get_declared_struct_member_size(spirv_type,
//             uint32_t(i));
//         mem->vec_size = memType.vecsize;
//         mem->columns = memType.columns;
//         mem->array_size = (memType.array.size() == 0) ? 1 : memType.array[0];
//         if (!first_member)
//             first_member = mem;
//         if (pre_member)
//             pre_member->next_member = mem;
//         pre_member = mem;
//         if (baseType == ShaderResourceBaseType::structure)
//             mem->members = read_shader_resource_members(compiler, memType);
//     }
//     return first_member;
// }

// FORCE_INLINE static void clean_shader_resource_member_info(
//     ShaderResourceMember* info) noexcept {
//     if (!info)
//         return;
//     memory::deque<ShaderResourceMember*, DefaultAlloc> queue{
//         get_default_alloc()};
//     while (!queue.empty()) {
//         auto const& top = queue.front();
//         if (top->members)
//             queue.push_front(top->members);
//         if (top->next_member)
//             queue.push_front(top->next_member);
//         get_default_alloc().destruct(top);
//         queue.pop_front();
//     }
// }

FORCE_INLINE static void get_shader_input(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.stage_inputs) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::input,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_base_type(compiler, res, out_res);
        read_shader_resource_vec_size(compiler, res, out_res);
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_location(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_input_attachment(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.subpass_inputs) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::input_attachment,
            .vk_access = VK_ACCESS_SHADER_READ_BIT,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_attachment_idx(compiler, res, out_res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_res);
        read_shader_resource_decoration_binding(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_output(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.stage_outputs) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::output,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_base_type(compiler, res, out_res);
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_vec_size(compiler, res, out_res);
        read_shader_resource_decoration_location(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_image(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.separate_images) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::image,
            .vk_access = VK_ACCESS_SHADER_READ_BIT,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_res);
        read_shader_resource_decoration_binding(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_image_sampler(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.sampled_images) {
        ShaderResource out_Res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::image_sampler,
            .vk_access = VK_ACCESS_SHADER_READ_BIT,
        };
        out_Res.vk_shader_stage |= stage;
        read_shader_resource_array_size(compiler, res, out_Res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_Res);
        read_shader_resource_decoration_binding(compiler, res, out_Res);
        out_shader_resources.push_back(std::move(out_Res));
    }
}

FORCE_INLINE static void get_shader_image_storage(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.storage_images) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::image_storage,
            // Initialization for query later
            .vk_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_decoration_nonreadable(compiler, res, out_res);
        read_shader_resource_decoration_nonwritable(compiler, res, out_res);
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_res);
        read_shader_resource_decoration_binding(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_sampler(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.separate_samplers) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::sampler,
            .vk_access = VK_ACCESS_SHADER_READ_BIT,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_res);
        read_shader_resource_decoration_binding(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_uniform_buffer(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const& desired_runtime_size,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.uniform_buffers) {
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::uniform_buffer,
            .vk_access = VK_ACCESS_UNIFORM_READ_BIT,
        };
        out_res.vk_shader_stage |= stage;
        // const auto& spirvType = compiler.get_type_from_variable(res.id);
        // out_res.members = read_shader_resource_members(compiler, spirvType);
        read_shader_resource_size(compiler, res, desired_runtime_size, out_res);
        read_shader_resource_array_size(compiler, res, out_res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_res);
        read_shader_resource_decoration_binding(compiler, res, out_res);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_storage_buffer(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const& desired_runtime_size,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.storage_buffers) {
        ShaderResource out_Res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::storage_buffer,
            .vk_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        };
        out_Res.vk_shader_stage |= stage;
        // const auto& spirvType = compiler.get_type_from_variable(res.id);
        // out_Res.members = read_shader_resource_members(compiler, spirvType);
        read_shader_resource_decoration_nonreadable(compiler, res, out_Res);
        read_shader_resource_decoration_nonwritable(compiler, res, out_Res);
        read_shader_resource_size(compiler, res, desired_runtime_size, out_Res);
        read_shader_resource_array_size(compiler, res, out_Res);
        read_shader_resource_decoration_descriptor_set(compiler, res, out_Res);
        read_shader_resource_decoration_binding(compiler, res, out_Res);
        out_shader_resources.push_back(std::move(out_Res));
    }
}

FORCE_INLINE static void get_shader_push_constant(
    spirv_cross::CompilerGLSL const& compiler,
    spirv_cross::ShaderResources const& spirv_resources,
    VkShaderStageFlagBits stage,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const& desired_runtime_size,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    for (auto& res : spirv_resources.push_constant_buffers) {
        const auto& spirvType = compiler.get_type_from_variable(res.id);
        uint32_t offset = std::numeric_limits<uint32_t>::max();
        // Get the start offset (offset of the whole struct in other words) of
        // the push constant buffer
        for (uint32_t i = 0u; i < spirvType.member_types.size(); ++i) {
            uint32_t memberOffset = compiler.get_member_decoration(
                spirvType.self, i, spv::DecorationOffset);
            offset = std::min(offset, memberOffset);
        }
        ShaderResource out_res{
            .name = res.name.c_str(),
            .type = ShaderResourceType::push_constant,
            .offset = offset,
        };
        out_res.vk_shader_stage |= stage;
        read_shader_resource_size(compiler, res, desired_runtime_size, out_res);
        out_res.size -= out_res.offset;
        // out_res.members = read_shader_resource_members(compiler, spirvType);
        out_shader_resources.push_back(std::move(out_res));
    }
}

FORCE_INLINE static void get_shader_specialization_constant(
    spirv_cross::CompilerGLSL const& compiler, VkShaderStageFlagBits stage,
    memory::vector<ShaderResource, DefaultAlloc>&
        out_shader_resources) noexcept {
    auto const& specializationConstant =
        compiler.get_specialization_constants();
    for (auto& res : specializationConstant) {
        ShaderResource out_res{
            .name = compiler.get_name(res.id).c_str(),
            .type = ShaderResourceType::specialization_constant,
            .constant_id = res.constant_id,
        };
        out_res.vk_shader_stage |= stage;
        const auto& constant = compiler.get_constant(res.id);
        const auto& spirvType = compiler.get_type(constant.constant_type);
        auto baseType = spirvType.basetype;
        out_res.base_type = spirv_type_to_shader_resource_base_type(baseType);
        out_res.size = get_spirv_base_type_size(baseType);
        out_shader_resources.push_back(std::move(out_res));
    }
}

auto spirv_reflection(std::span<const uint32_t> byte_code, int vk_shader_stage,
    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t,
        DefaultAlloc> const& desired_runtime_size) noexcept
    -> memory::vector<ShaderResource, DefaultAlloc> {
    try {
        spirv_cross::CompilerGLSL compiler{byte_code.data(), byte_code.size()};
        auto opt = compiler.get_common_options();
        opt.vulkan_semantics = true;
        compiler.set_common_options(opt);
        spirv_cross::ShaderResources resources =
            compiler.get_shader_resources();
        memory::vector<ShaderResource, DefaultAlloc> ret{get_default_alloc()};
        get_shader_input(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_input_attachment(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_output(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_image(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_sampler(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_image_sampler(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_image_storage(
            compiler, resources, (VkShaderStageFlagBits) vk_shader_stage, ret);
        get_shader_uniform_buffer(compiler, resources,
            (VkShaderStageFlagBits) vk_shader_stage, desired_runtime_size, ret);
        get_shader_storage_buffer(compiler, resources,
            (VkShaderStageFlagBits) vk_shader_stage, desired_runtime_size, ret);
        get_shader_push_constant(compiler, resources,
            (VkShaderStageFlagBits) vk_shader_stage, desired_runtime_size, ret);
        get_shader_specialization_constant(
            compiler, (VkShaderStageFlagBits) vk_shader_stage, ret);
        return ret;
    } catch (std::exception const& e) {
        COUST_PANIC_IF(true, "SpirV reflection failed: {}", e.what());
        ASSUME(0);
    }
}

}  // namespace detail
}  // namespace render
}  // namespace coust
