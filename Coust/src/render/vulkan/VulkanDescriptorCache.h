#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/SmartPtr.h"
#include "render/vulkan/VulkanDescriptor.h"
#include "render/vulkan/VulkanPipeline.h"
#include "render/vulkan/utils/CacheSetting.h"

namespace coust {
namespace render {

class VulkanDescriptorBuilder {
public:
    VulkanDescriptorBuilder(VulkanDescriptorBuilder&&) = delete;
    VulkanDescriptorBuilder(VulkanDescriptorBuilder const&) = delete;
    VulkanDescriptorBuilder& operator=(VulkanDescriptorBuilder&&) = delete;
    VulkanDescriptorBuilder& operator=(VulkanDescriptorBuilder const&) = delete;

public:
    VulkanDescriptorBuilder() noexcept = default;

    void gc() noexcept;

    void bind_shaders(std::span<const VulkanShaderModule*> modules) noexcept;

    void fill_requirements(
        std::span<VulkanDescriptorSetAllocator> allocators) noexcept;

    void bind_buffer(std::string_view name, class VulkanBuffer const& buffer,
        uint64_t offset, uint64_t size, uint32_t array_idx,
        bool suppress_warning) noexcept;

    void bind_image(std::string_view name, VkSampler sampler,
        class VulkanImage const& image, uint32_t array_idx) noexcept;

    void bind_input_attachment(std::string_view name,
        class VulkanAttachment const& attachment) noexcept;

    std::span<VulkanDescriptorSet::Param> get_params() noexcept;

private:
    memory::vector<VulkanDescriptorSet::Param, DefaultAlloc>
        m_descriptor_set_requirements{get_default_alloc()};

    std::span<const VulkanShaderModule*> m_related_shader_modules{};
};

class VulkanDescriptorCache {
public:
    VulkanDescriptorCache() = delete;
    VulkanDescriptorCache(VulkanDescriptorCache&&) = delete;
    VulkanDescriptorCache(VulkanDescriptorCache const&) = delete;
    VulkanDescriptorCache& operator=(VulkanDescriptorCache&&) = delete;
    VulkanDescriptorCache& operator=(VulkanDescriptorCache const&) = delete;

public:
    VulkanDescriptorCache(VkDevice dev, VkPhysicalDevice phy_dev) noexcept;

    void reset() noexcept;

    void gc() noexcept;

    const VulkanPipelineLayout* get_pipeline_layout(
        std::span<const VulkanShaderModule*> modules) noexcept;

    std::span<VulkanDescriptorSetAllocator> get_allocator(
        const VulkanPipelineLayout* layout) noexcept;

    void bind_descriptor_sets(VkCommandBuffer cmdbuf,
        VkPipelineBindPoint bind_point, VulkanPipelineLayout const& layout,
        std::span<const VulkanDescriptorSet::Param> params) noexcept;

private:
    memory::robin_map<VulkanPipelineLayout::Param,
        std::pair<memory::unique_ptr<VulkanPipelineLayout, DefaultAlloc>,
            uint32_t>,
        DefaultAlloc>
        m_pipeline_layouts{get_default_alloc()};

    memory::robin_map_nested<const VulkanPipelineLayout*,
        memory::vector<VulkanDescriptorSetAllocator, DefaultAlloc>,
        DefaultAlloc>
        m_descriptor_set_allocators{get_default_alloc()};

    memory::robin_map<VulkanDescriptorSet::Param,
        std::pair<VulkanDescriptorSet, uint32_t>, DefaultAlloc>
        m_descriptor_sets{get_default_alloc()};

    VkDevice m_dev = VK_NULL_HANDLE;

    VkPhysicalDevice m_phy_dev = VK_NULL_HANDLE;

    GCTimer m_gc_timer;

    CacheHitCounter m_hit_pipeline_layout_counter;
    CacheHitCounter m_hit_descriptor_set_counter;
};

}  // namespace render
}  // namespace coust
