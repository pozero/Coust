#include "pch.h"

#include "utils/Compiler.h"
#include "utils/math/Hash.h"
#include "utils/filesystem/NaiveSerialization.h"
#include "utils/filesystem/FileCache.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/VulkanShader.h"
#include "render/vulkan/utils/SpirVCompilation.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wexit-time-destructors")
CLANG_DISABLE_WARNING("-Wglobal-constructors")
memory::robin_map_nested<std::filesystem::path,
    std::pair<memory::string<DefaultAlloc>, size_t>, DefaultAlloc>
    ShaderSource::s_source_contents{get_default_alloc()};
WARNING_POP

ShaderSource::ShaderSource(std::filesystem::path path) noexcept
    : m_path(std::move(path)) {
}

memory::string<DefaultAlloc> const& ShaderSource::get_code() const noexcept {
    auto const iter = s_source_contents.find(m_path);
    if (iter != s_source_contents.end()) {
        return iter->second.first;
    } else {
        file::ByteArray byte_array = file::read_file_whole(m_path);
        memory::string<DefaultAlloc> content{
            byte_array.to_string_view(), get_default_alloc()};
        size_t const hash = calc_std_hash(content);
        auto const [iiter, inserted] = s_source_contents.emplace(
            m_path, std::make_pair(std::move(content), hash));
        return iiter->second.first;
    }
}

size_t ShaderSource::get_code_hash() const noexcept {
    auto const iter = s_source_contents.find(m_path);
    if (iter != s_source_contents.end()) {
        return iter->second.second;
    } else {
        file::ByteArray byte_array = file::read_file_whole(m_path);
        memory::string<DefaultAlloc> content{
            byte_array.to_string_view(), get_default_alloc()};
        size_t const hash = calc_std_hash(content);
        auto const [iiter, inserted] = s_source_contents.emplace(
            m_path, std::make_pair(std::move(content), hash));
        return iiter->second.second;
    }
}

void ShaderSource::add_macro(
    std::string_view name, std::string_view value) noexcept {
    memory::string<DefaultAlloc> nstr{name, get_default_alloc()};
    memory::string<DefaultAlloc> vstr{value, get_default_alloc()};
    m_macros.insert_or_assign(std::move(nstr), std::move(vstr));
}

void ShaderSource::set_dynamic_buffer_size(
    std::string_view name, size_t size) noexcept {
    memory::string<DefaultAlloc> nstr{name, get_default_alloc()};
    m_dynamic_buffer_size.insert_or_assign(std::move(nstr), size);
}

std::filesystem::path const& ShaderSource::get_path() const noexcept {
    return m_path;
}

bool ShaderSource::operator==(ShaderSource const& other) const noexcept {
    return m_path == other.m_path &&
           std::ranges::equal(m_macros, other.m_macros) &&
           std::ranges::equal(
               m_dynamic_buffer_size, other.m_dynamic_buffer_size);
}

auto ShaderSource::get_macros() const noexcept -> decltype(m_macros) const& {
    return m_macros;
}

auto ShaderSource::get_dynamic_buffer_sizes() const noexcept
    -> decltype(m_dynamic_buffer_size) const& {
    return m_dynamic_buffer_size;
}

VkDevice VulkanShaderModule::get_device() const noexcept {
    return m_dev;
}

VkShaderModule VulkanShaderModule::get_handle() const noexcept {
    return m_handle;
}

VulkanShaderModule::VulkanShaderModule(
    VkDevice dev, Param const& param) noexcept
    : m_dev(dev),
      m_source_path(param.source.get_path()),
      m_byte_code_cache_tag(calc_std_hash(param.source)),
      m_stage(param.stage) {
    {
        auto [byte_array, cache_status] =
            file::Caches::get_instance().get_cache_data(
                m_source_path.string(), m_byte_code_cache_tag);
        if (cache_status == file::Caches::Status::available) {
            file::from_byte_array(byte_array, m_byte_code);
        } else {
            m_flush_byte_code = true;
            m_byte_code = detail::compile_glst_to_spv(param.source, m_stage);
        }
    }
    {
        m_reflection_data_cache_tag = m_byte_code_cache_tag;
        uint32_t constexpr reflection_magic_val = 0x97538642;
        hash_combine(m_reflection_data_cache_tag, reflection_magic_val);
        for (auto const& [name, size] :
            param.source.get_dynamic_buffer_sizes()) {
            hash_combine(m_reflection_data_cache_tag, name);
            hash_combine(m_reflection_data_cache_tag, size);
        }
        auto [byte_array, cache_status] =
            file::Caches::get_instance().get_cache_data(
                m_source_path.string(), m_reflection_data_cache_tag);
        if (cache_status == file::Caches::Status::available) {
            file::from_byte_array(byte_array, m_reflection_data);
        } else {
            m_flush_reflection_data = true;
            m_reflection_data = detail::spirv_reflection(
                std::span<const uint32_t>{
                    m_byte_code.data(), m_byte_code.size()},
                m_stage, param.source.get_dynamic_buffer_sizes());
        }
    }
    VkShaderModuleCreateInfo module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = m_byte_code.size() * sizeof(uint32_t),
        .pCode = m_byte_code.data(),
    };
    COUST_VK_CHECK(vkCreateShaderModule(dev, &module_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "Can't create shader module from file {}", m_source_path.string());
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_byte_code(std::move(other.m_byte_code)),
      m_reflection_data(std::move(other.m_reflection_data)),
      m_source_path(std::move(other.m_source_path)),
      m_byte_code_cache_tag(other.m_byte_code_cache_tag),
      m_reflection_data_cache_tag(other.m_reflection_data_cache_tag),
      m_stage(other.m_stage),
      m_flush_byte_code(other.m_flush_byte_code),
      m_flush_reflection_data(other.m_flush_reflection_data) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanShaderModule& VulkanShaderModule::operator=(
    VulkanShaderModule&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    std::swap(m_byte_code, other.m_byte_code);
    std::swap(m_reflection_data, other.m_reflection_data);
    std::swap(m_source_path, other.m_source_path);
    std::swap(m_byte_code_cache_tag, other.m_byte_code_cache_tag);
    std::swap(m_reflection_data_cache_tag, other.m_reflection_data_cache_tag);
    std::swap(m_stage, other.m_stage);
    std::swap(m_flush_byte_code, other.m_flush_byte_code);
    std::swap(m_flush_reflection_data, other.m_flush_reflection_data);
    return *this;
}

VulkanShaderModule::~VulkanShaderModule() noexcept {
    if (m_handle) {
        if (m_flush_byte_code) {
            file::Caches::get_instance().add_cache_data(m_source_path.string(),
                m_byte_code_cache_tag, file::to_byte_array(m_byte_code), true);
        }
        if (m_flush_reflection_data) {
            file::Caches::get_instance().add_cache_data(m_source_path.string(),
                m_reflection_data_cache_tag,
                file::to_byte_array(m_reflection_data), true);
        }
        vkDestroyShaderModule(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

void VulkanShaderModule::set_dynamic_buffer(std::string_view name) noexcept {
    for (auto& res : m_reflection_data) {
        if (std::string_view{res.name} == name &&
            (res.type == ShaderResourceType::storage_buffer ||
                res.type == ShaderResourceType::uniform_buffer)) {
            res.update_mode = ShaderResourceUpdateMode::dyna;
            break;
        }
    }
}

VkShaderStageFlagBits VulkanShaderModule::get_stage() const noexcept {
    return m_stage;
}

std::pair<const uint32_t*, size_t> VulkanShaderModule::get_code()
    const noexcept {
    return std::make_pair(m_byte_code.data(), m_byte_code.size());
}

size_t VulkanShaderModule::get_byte_code_hash() const noexcept {
    return m_byte_code_cache_tag;
}

std::filesystem::path VulkanShaderModule::get_source_path() const noexcept {
    return m_source_path;
}

auto VulkanShaderModule::get_shader_resource() const noexcept
    -> decltype(m_reflection_data) const& {
    return m_reflection_data;
}

static_assert(detail::IsVulkanResource<VulkanShaderModule>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::ShaderSource>::operator()(
    coust::render::ShaderSource const& key) const noexcept {
    size_t hash = key.get_code_hash();
    for (auto const& [name, val] : key.get_macros()) {
        coust::hash_combine(hash, name);
        coust::hash_combine(hash, val);
    }
    return hash;
}

std::size_t hash<coust::render::VulkanShaderModule::Param>::operator()(
    coust::render::VulkanShaderModule::Param const& key) const noexcept {
    size_t hash = coust::calc_std_hash(key.source);
    coust::hash_combine(hash, key.stage);
    return hash;
}

bool equal_to<coust::render::VulkanShaderModule::Param>::operator()(
    coust::render::VulkanShaderModule::Param const& left,
    coust::render::VulkanShaderModule::Param const& right) const noexcept {
    return left.stage == right.stage &&
           left.source.get_code_hash() == right.source.get_code_hash() &&
           left.source.get_code() == right.source.get_code();
}

}  // namespace std
