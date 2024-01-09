#include "pch.h"

#include "render/vulkan/VulkanPipelineCache.h"
#include "render/vulkan/VulkanRenderTarget.h"

namespace coust {
namespace render {

VulkanGraphicsPipelineCache::VulkanGraphicsPipelineCache(VkDevice dev,
    VulkanShaderPool &shader_pool,
    VulkanDescriptorCache &descriptor_cache) noexcept
    : m_shader_pool(shader_pool),
      m_descriptor_cache(descriptor_cache),
      m_dev(dev),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD),
      m_graphics_pipeline_hit_counter(
          "Vulkan Graphics Pipeline Cache [VulkanGraphicsPipeline]") {
}

void VulkanGraphicsPipelineCache::reset() noexcept {
    m_graphics_pipelines.clear();
    m_cur_shader_modules.clear();
    m_cur_pipeline_layout = nullptr;
    m_cur_graphics_pipeline = nullptr;
    m_graphics_pipelines_requirement = {};
}

void VulkanGraphicsPipelineCache::gc(
    [[maybe_unused]] const VulkanCommandBuffer &buf) noexcept {
    m_gc_timer.tick();
    m_graphics_pipelines_requirement.special_const_info = {};
    m_cur_shader_modules.clear();
    m_cur_graphics_pipeline = nullptr;
    m_cur_pipeline_layout = nullptr;
    m_graphics_pipelines_requirement = {};
    for (auto iter = m_graphics_pipelines.begin();
         iter != m_graphics_pipelines.end();) {
        auto &[pipeline, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            iter = m_graphics_pipelines.erase(iter);
        } else {
            ++iter;
        }
    }
}

SpecializationConstantInfo &
    VulkanGraphicsPipelineCache::bind_specialization_constant() noexcept {
    return m_graphics_pipelines_requirement.special_const_info;
}

void VulkanGraphicsPipelineCache::bind_shader(
    VulkanShaderModule::Param const &param) noexcept {
    m_cur_shader_modules.push_back(m_shader_pool.get_shader(param));
}

void VulkanGraphicsPipelineCache::bind_pipeline_layout() noexcept {
    COUST_ASSERT(m_cur_pipeline_layout == nullptr, "");
    m_descriptor_builder.bind_shaders(m_cur_shader_modules);
    m_cur_pipeline_layout =
        m_descriptor_cache.get_pipeline_layout(m_cur_shader_modules);
    m_descriptor_builder.fill_requirements(
        m_descriptor_cache.get_allocator(m_cur_pipeline_layout));
}

void VulkanGraphicsPipelineCache::bind_raster_state(
    VulkanGraphicsPipeline::RasterState const &state) noexcept {
    m_graphics_pipelines_requirement.raster_state = state;
}

void VulkanGraphicsPipelineCache::bind_render_pass(
    VulkanRenderPass const &render_pass, uint32_t subpass) noexcept {
    m_graphics_pipelines_requirement.render_pass = &render_pass;
    m_graphics_pipelines_requirement.subpass = subpass;
}

void VulkanGraphicsPipelineCache::bind_buffer(std::string_view name,
    VulkanBuffer const &buffer, uint64_t offset, uint64_t size,
    uint32_t array_idx, bool suppress_warning) noexcept {
    m_descriptor_builder.bind_buffer(
        name, buffer, offset, size, array_idx, suppress_warning);
}

void VulkanGraphicsPipelineCache::bind_image(std::string_view name,
    VkSampler sampler, VulkanImage const &image, uint32_t array_idx) noexcept {
    m_descriptor_builder.bind_image(name, sampler, image, array_idx);
}

void VulkanGraphicsPipelineCache::bind_input_attachment(
    std::string_view name, class VulkanAttachment const &attachment) noexcept {
    m_descriptor_builder.bind_input_attachment(name, attachment);
}

void VulkanGraphicsPipelineCache::bind_descriptor_set(
    VkCommandBuffer cmdbuf) noexcept {
    std::span<VulkanDescriptorSet::Param> params =
        m_descriptor_builder.get_params();
    for (auto &param : params) {
        param.attached_cmdbuf = cmdbuf;
    }
    m_descriptor_cache.bind_descriptor_sets(cmdbuf,
        VK_PIPELINE_BIND_POINT_GRAPHICS, *m_cur_pipeline_layout, params);
}

void VulkanGraphicsPipelineCache::push_constant(VkCommandBuffer cmdbuf,
    VkShaderStageFlagBits stage, std::span<uint8_t const> data,
    uint32_t offset) noexcept {
    vkCmdPushConstants(cmdbuf, m_cur_pipeline_layout->get_handle(), stage,
        offset, (uint32_t) data.size(), data.data());
}

void VulkanGraphicsPipelineCache::bind_graphics_pipeline(
    VkCommandBuffer cmdBuf) noexcept {
    m_graphics_pipelines_requirement.shader_modules =
        std::span<VulkanShaderModule *>{
            m_cur_shader_modules.data(), m_cur_shader_modules.size()};
    auto iter = m_graphics_pipelines.find(m_graphics_pipelines_requirement);
    if (iter != m_graphics_pipelines.end()) {
        if (&iter->second.first == m_cur_graphics_pipeline)
            return;
        m_graphics_pipeline_hit_counter.hit();
        auto &[pipeline, last_accessed] = iter.mapped();
        m_cur_graphics_pipeline = &pipeline;
        last_accessed = m_gc_timer.current_count();
    } else {
        m_graphics_pipeline_hit_counter.miss();
        VulkanGraphicsPipeline pipeline{m_dev, *m_cur_pipeline_layout, m_cache,
            m_graphics_pipelines_requirement};
        auto [insert_iter, success] = m_graphics_pipelines.emplace(
            m_graphics_pipelines_requirement,
            std::make_pair(std::move(pipeline), m_gc_timer.current_count()));
        m_cur_graphics_pipeline = &insert_iter.mapped().first;
    }
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_cur_graphics_pipeline->get_handle());
}

memory::vector<VulkanShaderModule *, DefaultAlloc> &
    VulkanGraphicsPipelineCache::get_cur_shader_modules() noexcept {
    return m_cur_shader_modules;
}

VulkanComputePipelineCache::VulkanComputePipelineCache(VkDevice dev,
    VulkanShaderPool &shader_pool,
    VulkanDescriptorCache &descriptor_cache) noexcept
    : m_shader_pool(shader_pool),
      m_descriptor_cache(descriptor_cache),
      m_dev(dev),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD),
      m_compute_pipeline_hit_counter(
          "Vulkan Compute Pipeline Cache [VulkanComputePipeline]") {
}

void VulkanComputePipelineCache::reset() noexcept {
    m_compute_pipelines.clear();
    m_cur_shader_module = nullptr;
    m_cur_pipeline_layout = nullptr;
    m_cur_compute_pipeline = nullptr;
    m_specialzation_const_info = {};
}

void VulkanComputePipelineCache::gc(
    [[maybe_unused]] VulkanCommandBuffer const &buf) noexcept {
    m_gc_timer.tick();
    m_cur_shader_module = nullptr;
    m_cur_pipeline_layout = nullptr;
    m_cur_compute_pipeline = nullptr;
    m_specialzation_const_info = {};
    for (auto iter = m_compute_pipelines.begin();
         iter != m_compute_pipelines.end();) {
        auto &[pipeline, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            iter = m_compute_pipelines.erase(iter);
        } else {
            ++iter;
        }
    }
}

SpecializationConstantInfo &
    VulkanComputePipelineCache::bind_specialization_constant() noexcept {
    return m_specialzation_const_info;
}

void VulkanComputePipelineCache::bind_shader(
    VulkanShaderModule::Param const &param) noexcept {
    COUST_ASSERT(m_cur_shader_module == nullptr, "");
    m_cur_shader_module = m_shader_pool.get_shader(param);
}

void VulkanComputePipelineCache::bind_pipeline_layout() noexcept {
    COUST_ASSERT(m_cur_pipeline_layout == nullptr, "");
    m_descriptor_builder.bind_shaders({&m_cur_shader_module, 1});
    m_cur_pipeline_layout =
        m_descriptor_cache.get_pipeline_layout({&m_cur_shader_module, 1});
    m_descriptor_builder.fill_requirements(
        m_descriptor_cache.get_allocator(m_cur_pipeline_layout));
}

void VulkanComputePipelineCache::bind_buffer(std::string_view name,
    VulkanBuffer const &buffer, uint64_t offset, uint64_t size,
    uint32_t array_idx) noexcept {
    m_descriptor_builder.bind_buffer(
        name, buffer, offset, size, array_idx, false);
}

void VulkanComputePipelineCache::bind_image(std::string_view name,
    VkSampler sampler, VulkanImage const &image, uint32_t array_idx) noexcept {
    m_descriptor_builder.bind_image(name, sampler, image, array_idx);
}

void VulkanComputePipelineCache::bind_descriptor_set(
    VkCommandBuffer cmdbuf) noexcept {
    std::span<VulkanDescriptorSet::Param> params =
        m_descriptor_builder.get_params();
    for (auto &param : params) {
        param.attached_cmdbuf = cmdbuf;
    }
    m_descriptor_cache.bind_descriptor_sets(
        cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, *m_cur_pipeline_layout, params);
}

void VulkanComputePipelineCache::bind_compute_pipeline(
    VkCommandBuffer cmdbuf) noexcept {
    VulkanComputePipeline::Param param{
        .special_const_info = m_specialzation_const_info,
        .shader_module = m_cur_shader_module,
    };
    auto iter = m_compute_pipelines.find(param);
    if (iter != m_compute_pipelines.end()) {
        if (&iter.mapped().first == m_cur_compute_pipeline)
            return;
        m_compute_pipeline_hit_counter.hit();
        auto &[pipeline, last_accessed] = iter.mapped();
        m_cur_compute_pipeline = &pipeline;
        last_accessed = m_gc_timer.current_count();
    } else {
        m_compute_pipeline_hit_counter.miss();
        VulkanComputePipeline pipeline{
            m_dev, *m_cur_pipeline_layout, m_cache, param};
        auto [insert_iter, insert_success] = m_compute_pipelines.emplace(param,
            std::make_pair(std::move(pipeline), m_gc_timer.current_count()));
        m_cur_compute_pipeline = &insert_iter.mapped().first;
    }
    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
        m_cur_compute_pipeline->get_handle());
}

}  // namespace render
}  // namespace coust
