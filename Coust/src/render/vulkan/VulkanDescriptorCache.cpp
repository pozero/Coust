#include "pch.h"

#include "render/vulkan/VulkanDescriptorCache.h"
#include "render/vulkan/VulkanBuffer.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/VulkanRenderTarget.h"

namespace coust {
namespace render {

void VulkanDescriptorBuilder::gc() noexcept {
    m_descriptor_set_requirements.clear();
    m_related_shader_modules = {};
}

void VulkanDescriptorBuilder::bind_shaders(
    std::span<const VulkanShaderModule*> modules) noexcept {
    m_related_shader_modules = modules;
}

void VulkanDescriptorBuilder::fill_requirements(
    std::span<VulkanDescriptorSetAllocator> allocators) noexcept {
    uint32_t biggest_set = 0;
    for (auto const& allocator : allocators) {
        biggest_set = std::max(allocator.get_layout().get_set(), biggest_set);
    }
    m_descriptor_set_requirements.resize(biggest_set + 1);
    for (auto& allocator : allocators) {
        auto& requirement =
            m_descriptor_set_requirements[allocator.get_layout().get_set()];
        allocator.fill_empty_descriptor_set_param(requirement);
    }
}

void VulkanDescriptorBuilder::bind_buffer(std::string_view name,
    VulkanBuffer const& buffer, uint64_t offset, uint64_t size,
    uint32_t array_idx, bool suppress_warning) noexcept {
    COUST_ASSERT(m_related_shader_modules.size() > 0, "");
    for (auto const shader : m_related_shader_modules) {
        for (auto const& res : shader->get_shader_resource()) {
            if (res.name == name &&
                (res.type == ShaderResourceType::storage_buffer ||
                    res.type == ShaderResourceType::uniform_buffer)) {
                uint64_t offset_to_write = offset;
                COUST_PANIC_IF(
                    res.update_mode == ShaderResourceUpdateMode::dyna,
                    "Dynamic offset of buffer not supported yet");
                auto& requirement = m_descriptor_set_requirements[res.set];
                auto& arr = requirement.buffer_infos[res.binding];
                auto& buf_info = arr.buffers[array_idx];
                buf_info.buffer = buffer.get_handle();
                buf_info.offset = offset_to_write;
                buf_info.range = size;
                buf_info.dst_array_idx = array_idx;
                return;
            }
        }
    }
    if (!suppress_warning) {
        COUST_WARN("Can't find buffer named {} in the following shader:", name);
        for (auto const shader : m_related_shader_modules) {
            COUST_WARN("\t{}", shader->get_source_path().string());
        }
    }
}

void VulkanDescriptorBuilder::bind_image(std::string_view name,
    VkSampler sampler, class VulkanImage const& image,
    uint32_t array_idx) noexcept {
    COUST_ASSERT(m_related_shader_modules.size() > 0, "");
    for (auto const shader : m_related_shader_modules) {
        for (auto const& res : shader->get_shader_resource()) {
            if (res.name == name &&
                res.type == ShaderResourceType::image_sampler) {
                auto& requirement = m_descriptor_set_requirements[res.set];
                auto& arr = requirement.image_infos[res.binding];
                auto& img_info = arr.images[array_idx];
                img_info.sampler = sampler;
                img_info.image_view = image.get_primary_view().get_handle();
                img_info.image_layout = image.get_primary_layout();
                img_info.dst_array_idx = array_idx;
                return;
            }
        }
    }
    COUST_WARN("Can't find image named {} in the following shader:", name);
    for (auto const shader : m_related_shader_modules) {
        COUST_WARN("\t{}", shader->get_source_path().string());
    }
}

void VulkanDescriptorBuilder::bind_input_attachment(
    std::string_view name, class VulkanAttachment const& attachment) noexcept {
    COUST_ASSERT(m_related_shader_modules.size() > 0, "");
    for (auto const shader : m_related_shader_modules) {
        for (auto const& res : shader->get_shader_resource()) {
            if (res.name == name &&
                res.type == ShaderResourceType::input_attachment) {
                auto& requirement = m_descriptor_set_requirements[res.set];
                auto& arr = requirement.image_infos[res.binding];
                auto& img_info = arr.images[0];
                img_info.image_view =
                    attachment.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT)
                        ->get_handle();
                img_info.image_layout = attachment.get_layout();
                img_info.dst_array_idx = 0;
                return;
            }
        }
    }
    COUST_WARN(
        "Can't find input attachment named {} in the following shader:", name);
    for (auto const shader : m_related_shader_modules) {
        COUST_WARN("\t{}", shader->get_source_path().string());
    }
}

std::span<VulkanDescriptorSet::Param>
    VulkanDescriptorBuilder::get_params() noexcept {
    return m_descriptor_set_requirements;
}

VulkanDescriptorCache::VulkanDescriptorCache(
    VkDevice dev, VkPhysicalDevice phy_dev) noexcept
    : m_dev(dev),
      m_phy_dev(phy_dev),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD),
      m_hit_pipeline_layout_counter(
          "Vulkan Descirptor Cache [VulkanPipelineLayout]"),
      m_hit_descriptor_set_counter(
          "Vulkan Descriptor Cache [VulkanDescriptorSet]") {
}

void VulkanDescriptorCache::reset() noexcept {
    m_descriptor_sets.clear();
    m_descriptor_set_allocators.clear();
    m_pipeline_layouts.clear();
}

void VulkanDescriptorCache::gc() noexcept {
    m_gc_timer.tick();
    for (auto iter = m_descriptor_sets.begin();
         iter != m_descriptor_sets.end();) {
        auto& [set, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            iter = m_descriptor_sets.erase(iter);
        } else {
            ++iter;
        }
    }
    for (auto iter = m_pipeline_layouts.begin();
         iter != m_pipeline_layouts.end();) {
        auto& [layout, last_accessed] = iter->second;
        if (m_gc_timer.should_recycle(last_accessed)) {
            auto alloc_iter =
                m_descriptor_set_allocators.find(iter.mapped().first.get());
            COUST_ASSERT(alloc_iter != m_descriptor_set_allocators.end(), "");
            m_descriptor_set_allocators.erase(alloc_iter);
            iter = m_pipeline_layouts.erase(iter);
        } else {
            ++iter;
        }
    }
}

const VulkanPipelineLayout* VulkanDescriptorCache::get_pipeline_layout(
    std::span<const VulkanShaderModule*> modules) noexcept {
    VulkanPipelineLayout::Param param{modules};
    auto iter = m_pipeline_layouts.find(param);
    if (iter != m_pipeline_layouts.end()) {
        m_hit_pipeline_layout_counter.hit();
        auto& [layout, last_accessed] = iter.mapped();
        last_accessed = m_gc_timer.current_count();
        return layout.get();
    } else {
        m_hit_pipeline_layout_counter.miss();
        auto layout = memory::allocate_unique<VulkanPipelineLayout>(
            get_default_alloc(), m_dev, param);
        auto [layout_insert_iter, layout_insert_success] =
            m_pipeline_layouts.emplace(param,
                std::make_pair(std::move(layout), m_gc_timer.current_count()));
        COUST_ASSERT(layout_insert_success, "");
        const VulkanPipelineLayout* inserted_layout =
            layout_insert_iter.mapped().first.get();
        {
            auto [alloc_insert_iter, alloc_insert_success] =
                m_descriptor_set_allocators.emplace(inserted_layout,
                    memory::vector<VulkanDescriptorSetAllocator, DefaultAlloc>{
                        get_default_alloc()});
            COUST_ASSERT(layout_insert_success, "");
            for (auto const& descriptor_set_layout :
                inserted_layout->get_descriptor_set_layouts()) {
                alloc_insert_iter.mapped().emplace_back(
                    m_dev, descriptor_set_layout);
            }
        }
        return inserted_layout;
    }
}

std::span<VulkanDescriptorSetAllocator> VulkanDescriptorCache::get_allocator(
    const VulkanPipelineLayout* layout) noexcept {
    return m_descriptor_set_allocators.at(layout);
}

void VulkanDescriptorCache::bind_descriptor_sets(VkCommandBuffer cmdbuf,
    VkPipelineBindPoint bind_point, VulkanPipelineLayout const& layout,
    std::span<const VulkanDescriptorSet::Param> params) noexcept {
    memory::vector<VkDescriptorSet, DefaultAlloc> sets_to_bind{
        get_default_alloc()};
    sets_to_bind.reserve(params.size());
    // The descriptor requriement has already been sorted by set index. The
    // spec says: Values are taken from pDynamicOffsets in an order such that
    // all entries for set N come before set N+1; within a set, entries are
    // ordered by the binding numbers in the descriptor set layouts; and within
    // a binding array, elements are in order. dynamicOffsetCount must equal
    // the total number of dynamic descriptors in the sets being bound.
    for (auto& requirement : params) {
        COUST_ASSERT(requirement.attached_cmdbuf == cmdbuf, "");
        auto iter = m_descriptor_sets.find(requirement);
        if (iter != m_descriptor_sets.end()) {
            m_hit_descriptor_set_counter.hit();
            iter.mapped().second = m_gc_timer.current_count();
            auto const& [set, last_accessed] = iter.mapped();
            set.apply_write();
            sets_to_bind.push_back(set.get_handle());
        } else {
            m_hit_descriptor_set_counter.miss();
            VulkanDescriptorSet set{m_dev, m_phy_dev, requirement};
            set.apply_write();
            auto [insert_iter, success] = m_descriptor_sets.emplace(requirement,
                std::make_pair(std::move(set), m_gc_timer.current_count()));
            COUST_ASSERT(success, "");
            sets_to_bind.push_back(insert_iter.mapped().first.get_handle());
        }
    }
    if (!sets_to_bind.empty()) {
        vkCmdBindDescriptorSets(cmdbuf, bind_point, layout.get_handle(), 0,
            (uint32_t) sets_to_bind.size(), sets_to_bind.data(), 0, nullptr);
    }
}

}  // namespace render
}  // namespace coust
