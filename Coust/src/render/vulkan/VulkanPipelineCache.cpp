#include "pch.h"

#include "render/vulkan/VulkanPipelineCache.h"

namespace coust {
namespace render {

VulkanGraphicsPipelineCache::VulkanGraphicsPipelineCache(
    VkDevice dev, VkPhysicalDevice phy_dev) noexcept
    : m_dev(dev),
      m_phy_dev(phy_dev),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD),
      m_pipeline_layout_hit_counter(
          "Vulkan Graphics Pipeline Cache [VulkanPipelineLayout]"),
      m_graphics_pipeline_hit_counter(
          "Vulkan Graphics Pipeline Cache [VulkanGraphicsPipeline]"),
      m_descriptor_set_hit_counter(
          "Vulkan Graphics Pipeline Cache [VulkanDescriptorSet]") {
}

void VulkanGraphicsPipelineCache::reset() noexcept {
    m_shader_modules.clear();
    m_pipeline_layouts.clear();
    m_descriptor_set_allocators.clear();
    m_descriptor_sets.clear();
    m_graphics_pipelines.clear();
    m_cur_shader_modules.clear();
    m_cur_pipeline_layout = nullptr;
    m_cur_graphics_pipeline = nullptr;
    m_cur_descriptor_sets.clear();
    m_dynamic_offsets.clear();
    m_pipeline_layout_requirement = {};
    m_graphics_pipelines_requirement = {};
    m_descriptor_set_requirements.clear();
}

void VulkanGraphicsPipelineCache::gc(
    [[maybe_unused]] const VulkanCommandBuffer &buf) noexcept {
    m_gc_timer.tick();
    m_graphics_pipelines_requirement.special_const_info = {};
    m_cur_shader_modules.clear();
    m_cur_descriptor_sets.clear();
    m_dynamic_offsets.clear();
    m_cur_graphics_pipeline = nullptr;
    m_cur_pipeline_layout = nullptr;
    for (auto iter = m_descriptor_sets.begin();
         iter != m_descriptor_sets.end();) {
        auto &[set, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            iter = m_descriptor_sets.erase(iter);
        } else {
            ++iter;
        }
    }
    for (auto iter = m_graphics_pipelines.begin();
         iter != m_graphics_pipelines.end();) {
        auto &[pipeline, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            iter = m_graphics_pipelines.erase(iter);
        } else {
            ++iter;
        }
    }
    for (auto iter = m_pipeline_layouts.begin();
         iter != m_pipeline_layouts.end();) {
        auto &[layout, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            auto alloc_iter =
                m_descriptor_set_allocators.find(&iter.mapped().first);
            m_descriptor_set_allocators.erase(alloc_iter);
            iter = m_pipeline_layouts.erase(iter);
        } else {
            ++iter;
        }
    }
}

void VulkanGraphicsPipelineCache::unbind_descriptor_set() noexcept {
    m_descriptor_set_requirements.clear();
}

void VulkanGraphicsPipelineCache::unbind_pipeline() noexcept {
    m_pipeline_layout_requirement = {};
    m_graphics_pipelines_requirement = {};
}

SpecializationConstantInfo &
    VulkanGraphicsPipelineCache::bind_specialization_constant() noexcept {
    return m_graphics_pipelines_requirement.special_const_info;
}

bool VulkanGraphicsPipelineCache::bind_shader(
    VulkanShaderModule::Param const &param,
    std::span<std::string_view> dynamic_buffer_names) noexcept {
    auto iter = m_shader_modules.find(param);
    if (iter != m_shader_modules.end()) {
        m_cur_shader_modules.push_back(&iter.mapped());
        return true;
    } else {
        VulkanShaderModule shader_module{m_dev, param};
        for (auto const name : dynamic_buffer_names) {
            shader_module.set_dynamic_buffer(name);
        }
        auto [emplace_iter, success] =
            m_shader_modules.emplace(param, std::move(shader_module));
        COUST_PANIC_IF_NOT(success, "");
        m_cur_shader_modules.push_back(&emplace_iter.mapped());
        return false;
    }
}

bool VulkanGraphicsPipelineCache::bind_pipeline_layout() noexcept {
    m_pipeline_layout_requirement.shader_modules =
        std::span<const VulkanShaderModule *>{
            m_cur_shader_modules.data(), m_cur_shader_modules.size()};
    auto iter = m_pipeline_layouts.find(m_pipeline_layout_requirement);
    if (iter != m_pipeline_layouts.end()) {
        m_pipeline_layout_hit_counter.hit();
        auto &[layout, last_accessed] = iter.mapped();
        m_cur_pipeline_layout = &layout;
        last_accessed = m_gc_timer.current_count();
        fill_descriptor_set_requirements();
        return true;
    } else {
        m_pipeline_layout_hit_counter.miss();
        VulkanPipelineLayout layout{m_dev, m_pipeline_layout_requirement};
        auto [insert_iter, success] =
            m_pipeline_layouts.emplace(m_pipeline_layout_requirement,
                std::make_pair(std::move(layout), m_gc_timer.current_count()));
        COUST_PANIC_IF_NOT(success, "");
        m_cur_pipeline_layout = &insert_iter.mapped().first;
        create_descriptor_allocators();
        fill_descriptor_set_requirements();
        return false;
    }
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
    uint32_t array_idx) noexcept {
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        for (auto const &res : shader->get_shader_resource()) {
            if (res.name == name &&
                (res.type == ShaderResourceType::storage_buffer ||
                    res.type == ShaderResourceType::uniform_buffer)) {
                uint64_t offset_to_write = offset;
                if (res.update_mode == ShaderResourceUpdateMode::dyna) {
                    offset_to_write = 0;
                    m_dynamic_offsets.insert_or_assign(
                        res.set, (uint32_t) offset);
                }
                auto &requirement = m_descriptor_set_requirements[res.set];
                auto &arr = requirement.buffer_infos[res.binding];
                auto &buf_info = arr.buffers[array_idx];
                buf_info.buffer = buffer.get_handle();
                buf_info.offset = offset_to_write;
                buf_info.range = size;
                buf_info.dst_array_idx = array_idx;
                return;
            }
        }
    }
    COUST_WARN("Can't find buffer named {} in the following shader:", name);
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        COUST_WARN("\t{}", shader->get_source_path().string());
    }
}

void VulkanGraphicsPipelineCache::bind_image(std::string_view name,
    VkSampler sampler, VulkanImage const &image, uint32_t array_idx) noexcept {
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        for (auto const &res : shader->get_shader_resource()) {
            if (res.name == name &&
                res.type == ShaderResourceType::image_sampler) {
                auto &requirement = m_descriptor_set_requirements[res.set];
                auto &arr = requirement.image_infos[res.binding];
                auto &img_info = arr.images[array_idx];
                img_info.sampler = sampler;
                img_info.image_view = image.get_primary_view().get_handle();
                img_info.image_layout = image.get_primary_layout();
                img_info.dst_array_idx = array_idx;
                return;
            }
        }
    }
    COUST_WARN("Can't find image named {} in the following shader:", name);
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        COUST_WARN("\t{}", shader->get_source_path().string());
    }
}

void VulkanGraphicsPipelineCache::bind_input_attachment(
    std::string_view name, VulkanImage const &attachment) noexcept {
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        for (auto const &res : shader->get_shader_resource()) {
            if (res.name == name &&
                res.type == ShaderResourceType::input_attachment) {
                auto &requirement = m_descriptor_set_requirements[res.set];
                auto &arr = requirement.image_infos[res.binding];
                auto &img_info = arr.images[0];
                img_info.image_view =
                    attachment.get_primary_view().get_handle();
                img_info.image_layout = attachment.get_primary_layout();
                img_info.dst_array_idx = 0;
                return;
            }
        }
    }
    COUST_WARN(
        "Can't find input attachment named {} in the following shader:", name);
    for (auto const shader : m_pipeline_layout_requirement.shader_modules) {
        COUST_WARN("\t{}", shader->get_source_path().string());
    }
}

bool VulkanGraphicsPipelineCache::bind_descriptor_set(
    VkCommandBuffer cmdBuf) noexcept {
    memory::vector<VkDescriptorSet, DefaultAlloc> sets_to_bind{
        get_default_alloc()};
    sets_to_bind.reserve(m_descriptor_set_requirements.size());
    memory::vector<uint32_t, DefaultAlloc> dynamic_offsets{get_default_alloc()};
    dynamic_offsets.reserve(m_dynamic_offsets.size());
    // The descriptor requriement has already been sorted by set index. The spec
    // says: Values are taken from pDynamicOffsets in an order such that all
    // entries for set N come before set N+1; within a set, entries are ordered
    // by the binding numbers in the descriptor set layouts; and within a
    // binding array, elements are in order. dynamicOffsetCount must equal the
    // total number of dynamic descriptors in the sets being bound.
    for (auto &requirement : m_descriptor_set_requirements) {
        auto iter = m_descriptor_sets.find(requirement);
        if (iter != m_descriptor_sets.end()) {
            for (auto const set : m_cur_descriptor_sets) {
                if (&iter.mapped().first == set)
                    continue;
            }
            m_descriptor_set_hit_counter.hit();
            auto &[set, last_accessed] = iter.mapped();
            set.apply_write(true);
            last_accessed = m_gc_timer.current_count();
            m_cur_descriptor_sets.push_back(&set);
            sets_to_bind.push_back(set.get_handle());
        } else {
            m_descriptor_set_hit_counter.miss();
            VulkanDescriptorSet set{m_dev, m_phy_dev, requirement};
            set.apply_write(true);
            auto [insert_iter, success] = m_descriptor_sets.emplace(requirement,
                std::make_pair(std::move(set), m_gc_timer.current_count()));
            COUST_PANIC_IF_NOT(success, "");
            m_cur_descriptor_sets.push_back(&insert_iter.mapped().first);
            sets_to_bind.push_back(set.get_handle());
        }
        if (m_dynamic_offsets.contains(requirement.set)) {
            dynamic_offsets.push_back(m_dynamic_offsets.at(requirement.set));
        }
    }
    if (!sets_to_bind.empty()) {
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_cur_pipeline_layout->get_handle(), 0,
            (uint32_t) sets_to_bind.size(), sets_to_bind.data(),
            (uint32_t) dynamic_offsets.size(), dynamic_offsets.data());
    }
    return true;
}

bool VulkanGraphicsPipelineCache::bind_graphics_pipeline(
    VkCommandBuffer cmdBuf) noexcept {
    auto iter = m_graphics_pipelines.find(m_graphics_pipelines_requirement);
    if (iter != m_graphics_pipelines.end()) {
        if (&iter->second.first == m_cur_graphics_pipeline)
            return true;
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
    return true;
}

void VulkanGraphicsPipelineCache::create_descriptor_allocators() noexcept {
    auto [iter, success] =
        m_descriptor_set_allocators.emplace(m_cur_pipeline_layout,
            memory::vector<VulkanDescriptorSetAllocator, DefaultAlloc>{
                get_default_alloc()});
    COUST_PANIC_IF_NOT(success, "");
    for (auto const &layout :
        m_cur_pipeline_layout->get_descriptor_set_layouts()) {
        iter.mapped().emplace_back(m_dev, layout);
    }
}

void VulkanGraphicsPipelineCache::fill_descriptor_set_requirements() noexcept {
    m_descriptor_set_requirements.clear();
    auto &allocators = m_descriptor_set_allocators.at(m_cur_pipeline_layout);
    uint32_t biggest_set = 0;
    for (auto const &alloc : allocators) {
        biggest_set = std::max(alloc.get_layout().get_set(), biggest_set);
    }
    m_descriptor_set_requirements.resize(biggest_set + 1);
    for (auto &alloc : allocators) {
        auto &requirement =
            m_descriptor_set_requirements[alloc.get_layout().get_set()];
        alloc.fill_empty_descriptor_set_param(requirement);
    }
}

}  // namespace render
}  // namespace coust
