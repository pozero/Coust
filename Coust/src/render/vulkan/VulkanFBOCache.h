#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "render/vulkan/utils/CacheSetting.h"
#include "render/vulkan/VulkanRenderPass.h"
#include "render/vulkan/VulkanFramebuffer.h"

namespace coust {
namespace render {

class VulkanFBOCache {
public:
    VulkanFBOCache() = delete;
    VulkanFBOCache(VulkanFBOCache &&) = delete;
    VulkanFBOCache(VulkanFBOCache const &) = delete;
    VulkanFBOCache &operator=(VulkanFBOCache &&) = delete;
    VulkanFBOCache &operator=(VulkanFBOCache const &) = delete;

public:
    VulkanFBOCache(VkDevice dev) noexcept;

    VulkanRenderPass const &get_render_pass(
        VulkanRenderPass::Param const &param) noexcept;

    VulkanFramebuffer const &get_render_pass(
        VulkanRenderPass const &render_pass,
        VulkanFramebuffer::Param const &param) noexcept;

    void gc() noexcept;

    void reset() noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    memory::robin_map<VulkanRenderPass::Param,
        std::pair<VulkanRenderPass, uint32_t>, DefaultAlloc>
        m_render_passes{get_default_alloc()};

    memory::robin_map<VulkanFramebuffer::Param,
        std::pair<VulkanFramebuffer, uint32_t>, DefaultAlloc>
        m_framebuffer{get_default_alloc()};

    memory::robin_map<const VulkanRenderPass *, uint32_t, DefaultAlloc>
        m_render_pass_ref_counts{get_default_alloc()};

    CacheHitCounter m_render_pass_hit_counter;
    CacheHitCounter m_framebuffer_hit_counter;

    GCTimer m_gc_timer;
};

}  // namespace render
}  // namespace coust
