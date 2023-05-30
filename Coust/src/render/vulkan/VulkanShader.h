#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/SmartPtr.h"
#include "utils/containers/RobinMap.h"
#include "utils/filesystem/FileIO.h"
#include "render/vulkan/utils/IncompleteVulkan.h"
#include "render/vulkan/utils/SpirVReflection.h"

VK_DEFINE_HANDLE(VkShaderModule)

namespace coust {
namespace render {

class ShaderSource {
public:
    ShaderSource() = delete;

    ShaderSource(ShaderSource&&) noexcept = default;
    ShaderSource(ShaderSource const&) noexcept = default;
    ShaderSource& operator=(ShaderSource&&) noexcept = default;
    ShaderSource& operator=(ShaderSource const&) noexcept = default;

public:
    static memory::robin_map_nested<std::filesystem::path,
        std::pair<memory::string<DefaultAlloc>, size_t>, DefaultAlloc>
        s_source_contents;

public:
    explicit ShaderSource(std::filesystem::path path) noexcept;

    memory::string<DefaultAlloc> const& get_code() const noexcept;

    size_t get_code_hash() const noexcept;

    void add_macro(std::string_view name, std::string_view value) noexcept;

    void set_dynamic_buffer_size(std::string_view name, size_t size) noexcept;

    std::filesystem::path const& get_path() const noexcept;

    bool operator==(ShaderSource const& other) const noexcept;

private:
    std::filesystem::path m_path{};

    memory::robin_map_nested<memory::string<DefaultAlloc>,
        memory::string<DefaultAlloc>, DefaultAlloc>
        m_macros{get_default_alloc()};

    memory::robin_map_nested<memory::string<DefaultAlloc>, size_t, DefaultAlloc>
        m_dynamic_buffer_size{get_default_alloc()};

public:
    auto get_macros() const noexcept -> decltype(m_macros) const&;

    auto get_dynamic_buffer_sizes() const noexcept
        -> decltype(m_dynamic_buffer_size) const&;
};

class VulkanShaderModule {
public:
    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanShaderModule&&) = delete;
    VulkanShaderModule(VulkanShaderModule const&) = delete;
    VulkanShaderModule& operator=(VulkanShaderModule&&) = delete;
    VulkanShaderModule& operator=(VulkanShaderModule const&) = delete;

public:
    // VK_OBJECT_TYPE_SHADER_MODULE
    static int constexpr object_type = 15;
    VkDevice get_device() const noexcept;
    VkShaderModule get_handle() const noexcept;

    struct Param {
        int vk_shader_stage;
        ShaderSource source;
    };

public:
    VulkanShaderModule(VkDevice dev, Param const& param) noexcept;

    ~VulkanShaderModule() noexcept;

    int get_stage() const noexcept;

    std::pair<const uint32_t*, size_t> get_code() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;
    VkShaderModule m_handle = VK_NULL_HANDLE;

    memory::vector<uint32_t, DefaultAlloc> m_byte_code{get_default_alloc()};

    memory::vector<ShaderResource, DefaultAlloc> m_reflection_data{
        get_default_alloc()};

    std::filesystem::path m_source_path{};

    size_t m_byte_code_cache_tag = 0;

    size_t m_reflection_data_cache_tag = 0;

    int m_vk_shader_stage = 0;

    bool m_flush_byte_code = false;

    bool m_flush_reflection_data = false;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::ShaderSource> {
    std::size_t operator()(
        coust::render::ShaderSource const& key) const noexcept;
};

template <>
struct hash<coust::render::VulkanShaderModule::Param> {
    std::size_t operator()(
        coust::render::VulkanShaderModule::Param const& key) const noexcept;
};

}  // namespace std
