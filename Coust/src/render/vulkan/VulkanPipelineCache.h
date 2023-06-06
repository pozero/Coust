#pragma once

#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"
#include "render/vulkan/VulkanPipeline.h"
#include "render/vulkan/VulkanCommand.h"
#include "render/vulkan/VulkanBuffer.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/utils/CacheSetting.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanGraphicsPipelineCache {
public:
    VulkanGraphicsPipelineCache() = delete;
    VulkanGraphicsPipelineCache(VulkanGraphicsPipelineCache &&) = delete;
    VulkanGraphicsPipelineCache(VulkanGraphicsPipelineCache const &) = delete;
    VulkanGraphicsPipelineCache &operator=(
        VulkanGraphicsPipelineCache &&) = delete;
    VulkanGraphicsPipelineCache &operator=(
        VulkanGraphicsPipelineCache const &) = delete;

public:
    explicit VulkanGraphicsPipelineCache(
        VkDevice dev, VkPhysicalDevice phy_dev) noexcept;

    // Clean every thing that's been cached
    void reset() noexcept;

    // This function will be called when the driver switches to a new command
    // buffer, which means the old command buffer is submitted. We know that
    // command buffer records all these bindings, so once it's submitted, all
    // the old render states bound previously are gone. We would do some
    // housekeeping here, clearing out all the old render states.
    void gc(const VulkanCommandBuffer &buf) noexcept;

    // Internal clearing methods. They just set the corresponding requirements
    // to default state.

    void unbind_descriptor_set() noexcept;

    void unbind_pipeline() noexcept;

    // Internal binding methods. They don't issue any vulkan call.
    // Since we use reflection to manage our resource binding, some of the
    // following functions require strict calling order. Basically, all function
    // related to shader resoure binding should be called AFTER the binding of
    // pipeline layout (which contains descriptor set layout) finished, and
    // actual vulkan binding should always happens at the very end. Ideally, the
    // call order should be the same as the order they present below.

    // Get the specialization constant info and modify it.
    // Specialization constant info will get cleared when a new command buffer
    // is activated.
    SpecializationConstantInfo &bind_specialization_constant() noexcept;

    bool bind_shader(VulkanShaderModule::Param const &param,
        std::span<std::string_view> dynamic_buffer_names) noexcept;

    // Tell the cache that's all the shader resource (with correct update mode)
    // we need, and build or get the pipeline layout and corresponding
    // descriptor set allocator.
    bool bind_pipeline_layout() noexcept;

    void bind_raster_state(
        VulkanGraphicsPipeline::RasterState const &state) noexcept;

    void bind_render_pass(
        VulkanRenderPass const &render_pass, uint32_t subpass) noexcept;

    // The following functions are responsible for configuring the construction
    // parameter for descriptor sets

    void bind_buffer(std::string_view name, VulkanBuffer const &buffer,
        uint64_t offset, uint64_t size, uint32_t arrayIdx) noexcept;

    void bind_image(std::string_view name, VkSampler sampler,
        VulkanImage const &image, uint32_t arrayIdx) noexcept;

    void bind_input_attachment(std::string_view name,
        class VulkanAttachment const &attachment) noexcept;

    // Actual binding

    bool bind_descriptor_set(VkCommandBuffer cmdbuf) noexcept;

    bool bind_graphics_pipeline(VkCommandBuffer cmdbuf) noexcept;

private:
    void create_descriptor_allocators() noexcept;

    void fill_descriptor_set_requirements() noexcept;

private:
    memory::robin_map<VulkanShaderModule::Param,
        memory::unique_ptr<VulkanShaderModule, DefaultAlloc>, DefaultAlloc>
        m_shader_modules{get_default_alloc()};

    memory::robin_map<VulkanPipelineLayout::Param,
        std::pair<VulkanPipelineLayout, uint32_t>, DefaultAlloc>
        m_pipeline_layouts{get_default_alloc()};

    memory::robin_map_nested<const VulkanPipelineLayout *,
        memory::vector<VulkanDescriptorSetAllocator, DefaultAlloc>,
        DefaultAlloc>
        m_descriptor_set_allocators{get_default_alloc()};

    memory::robin_map<VulkanDescriptorSet::Param,
        std::pair<VulkanDescriptorSet, uint32_t>, DefaultAlloc>
        m_descriptor_sets{get_default_alloc()};

    memory::robin_map<VulkanGraphicsPipeline::Param,
        std::pair<VulkanGraphicsPipeline, uint32_t>, DefaultAlloc>
        m_graphics_pipelines{get_default_alloc()};

    memory::vector<const VulkanShaderModule *, DefaultAlloc>
        m_cur_shader_modules{get_default_alloc()};

    const VulkanPipelineLayout *m_cur_pipeline_layout = nullptr;

    const VulkanGraphicsPipeline *m_cur_graphics_pipeline = nullptr;

    memory::robin_map<uint32_t, uint32_t, DefaultAlloc> m_dynamic_offsets{
        get_default_alloc()};

    VulkanPipelineLayout::Param m_pipeline_layout_requirement{};

    VulkanGraphicsPipeline::Param m_graphics_pipelines_requirement{};

    memory::vector<VulkanDescriptorSet::Param, DefaultAlloc>
        m_descriptor_set_requirements{get_default_alloc()};

    VkDevice m_dev = VK_NULL_HANDLE;
    VkPhysicalDevice m_phy_dev = VK_NULL_HANDLE;

    VkPipelineCache m_cache = VK_NULL_HANDLE;

    GCTimer m_gc_timer;

    CacheHitCounter m_pipeline_layout_hit_counter;
    CacheHitCounter m_graphics_pipeline_hit_counter;
    CacheHitCounter m_descriptor_set_hit_counter;
};

}  // namespace render
}  // namespace coust
