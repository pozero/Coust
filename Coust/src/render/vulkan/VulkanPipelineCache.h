#pragma once

#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"
#include "render/vulkan/VulkanPipeline.h"
#include "render/vulkan/VulkanCommand.h"
#include "render/vulkan/VulkanBuffer.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/VulkanDescriptorCache.h"
#include "render/vulkan/VulkanShaderPool.h"
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
    explicit VulkanGraphicsPipelineCache(VkDevice dev,
        VulkanShaderPool &shader_pool,
        VulkanDescriptorCache &descriptor_cache) noexcept;

    void reset() noexcept;

    void gc(const VulkanCommandBuffer &buf) noexcept;

    SpecializationConstantInfo &bind_specialization_constant() noexcept;

    void bind_shader(VulkanShaderModule::Param const &param) noexcept;

    void bind_pipeline_layout() noexcept;

    void bind_raster_state(
        VulkanGraphicsPipeline::RasterState const &state) noexcept;

    void bind_render_pass(
        VulkanRenderPass const &render_pass, uint32_t subpass) noexcept;

    void bind_buffer(std::string_view name, VulkanBuffer const &buffer,
        uint64_t offset, uint64_t size, uint32_t array_idx,
        bool suppress_warning) noexcept;

    void bind_image(std::string_view name, VkSampler sampler,
        VulkanImage const &image, uint32_t array_idx) noexcept;

    void bind_input_attachment(std::string_view name,
        class VulkanAttachment const &attachment) noexcept;

    void bind_descriptor_set(VkCommandBuffer cmdbuf) noexcept;

    void bind_graphics_pipeline(VkCommandBuffer cmdbuf) noexcept;

private:
    VulkanShaderPool &m_shader_pool;

    VulkanDescriptorCache &m_descriptor_cache;

    memory::robin_map<VulkanGraphicsPipeline::Param,
        std::pair<VulkanGraphicsPipeline, uint32_t>, DefaultAlloc>
        m_graphics_pipelines{get_default_alloc()};

    VulkanDescriptorBuilder m_descriptor_builder;

    memory::vector<const VulkanShaderModule *, DefaultAlloc>
        m_cur_shader_modules{get_default_alloc()};

    const VulkanPipelineLayout *m_cur_pipeline_layout = nullptr;

    const VulkanGraphicsPipeline *m_cur_graphics_pipeline = nullptr;

    VulkanGraphicsPipeline::Param m_graphics_pipelines_requirement{};

    VkDevice m_dev = VK_NULL_HANDLE;

    VkPipelineCache m_cache = VK_NULL_HANDLE;

    GCTimer m_gc_timer;

    CacheHitCounter m_graphics_pipeline_hit_counter;
};

class VulkanComputePipelineCache {
public:
    VulkanComputePipelineCache() = delete;
    VulkanComputePipelineCache(VulkanComputePipelineCache &&) = delete;
    VulkanComputePipelineCache(VulkanComputePipelineCache const &) = delete;
    VulkanComputePipelineCache &operator=(
        VulkanComputePipelineCache &&) = delete;
    VulkanComputePipelineCache &operator=(
        VulkanComputePipelineCache const &) = delete;

public:
    VulkanComputePipelineCache(VkDevice dev, VulkanShaderPool &shader_pool,
        VulkanDescriptorCache &descriptor_cache) noexcept;

    void reset() noexcept;

    void gc(VulkanCommandBuffer const &buf) noexcept;

    SpecializationConstantInfo &bind_specialization_constant() noexcept;

    void bind_shader(VulkanShaderModule::Param const &param) noexcept;

    void bind_pipeline_layout() noexcept;

    void bind_buffer(std::string_view name, VulkanBuffer const &buffer,
        uint64_t offset, uint64_t size, uint32_t array_idx) noexcept;

    void bind_image(std::string_view name, VkSampler sampler,
        VulkanImage const &image, uint32_t arrayIdx) noexcept;

    void bind_descriptor_set(VkCommandBuffer cmdbuf) noexcept;

    void bind_compute_pipeline(VkCommandBuffer cmdbuf) noexcept;

private:
    VulkanShaderPool &m_shader_pool;

    VulkanDescriptorCache &m_descriptor_cache;

    memory::robin_map<VulkanComputePipeline::Param,
        std::pair<VulkanComputePipeline, uint32_t>, DefaultAlloc>
        m_compute_pipelines{get_default_alloc()};

    VulkanDescriptorBuilder m_descriptor_builder;

    const VulkanShaderModule *m_cur_shader_module = nullptr;

    const VulkanPipelineLayout *m_cur_pipeline_layout = nullptr;

    const VulkanComputePipeline *m_cur_compute_pipeline = nullptr;

    SpecializationConstantInfo m_specialzation_const_info{};

    VkDevice m_dev = VK_NULL_HANDLE;

    VkPipelineCache m_cache = VK_NULL_HANDLE;

    GCTimer m_gc_timer;

    CacheHitCounter m_compute_pipeline_hit_counter;
};

}  // namespace render
}  // namespace coust
