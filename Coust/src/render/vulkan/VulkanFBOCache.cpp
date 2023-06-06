#include "pch.h"

#include "render/vulkan/VulkanFBOCache.h"

namespace coust {
namespace render {

VulkanFBOCache::VulkanFBOCache(VkDevice dev) noexcept
    : m_dev(dev),
      m_render_pass_hit_counter("Vulkan FBO Cache [Render Pass]"),
      m_framebuffer_hit_counter("Vulkan FBO Cache [Framebuffer]"),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD) {
}

VulkanRenderPass const &VulkanFBOCache::get_render_pass(
    VulkanRenderPass::Param const &param) noexcept {
    auto iter = m_render_passes.find(param);
    if (iter != m_render_passes.end()) {
        m_render_pass_hit_counter.hit();
        auto &[render_pass, last_access] = iter.mapped();
        last_access = m_gc_timer.current_count();
        return render_pass;
    } else {
        m_render_pass_hit_counter.miss();
        auto [insert_iter, success] = m_render_passes.emplace(
            param, std::make_pair(VulkanRenderPass{m_dev, param},
                       m_gc_timer.current_count()));
        COUST_PANIC_IF_NOT(success, "");
        auto const &[render_pass, last_access] = insert_iter.mapped();
        m_render_pass_ref_counts.emplace(&render_pass, 0);
        return render_pass;
    }
}

VulkanFramebuffer const &VulkanFBOCache::get_framebuffer(
    VulkanFramebuffer::Param const &param) noexcept {
    auto iter = m_framebuffer.find(param);
    if (iter != m_framebuffer.end()) {
        m_framebuffer_hit_counter.hit();
        auto &[framebuffer, last_access] = iter.mapped();
        last_access = m_gc_timer.current_count();
        return framebuffer;
    } else {
        m_framebuffer_hit_counter.miss();
        VulkanFramebuffer framebuffer{m_dev, param};
        auto [insert_iter, success] = m_framebuffer.emplace(param,
            std::make_pair(std::move(framebuffer), m_gc_timer.current_count()));
        COUST_PANIC_IF_NOT(success, "");
        m_render_pass_ref_counts.at(param.render_pass) += 1;
        auto const &[fb, last_access] = insert_iter.mapped();
        return fb;
    }
}

void VulkanFBOCache::gc() noexcept {
    m_gc_timer.tick();
    for (auto iter = m_framebuffer.begin(); iter != m_framebuffer.end();) {
        auto const &[framebuffer, last_access] = iter.mapped();
        if (m_gc_timer.should_recycle(last_access)) {
            m_render_pass_ref_counts.at(&framebuffer.get_render_pass()) -= 1;
            iter = m_framebuffer.erase(iter);
        } else {
            ++iter;
        }
    }
    for (auto iter = m_render_passes.begin(); iter != m_render_passes.end();) {
        auto const &[render_pass, last_access] = iter.mapped();
        if (m_gc_timer.should_recycle(last_access) &&
            m_render_pass_ref_counts.at(&render_pass) == 0) {
            iter = m_render_passes.erase(iter);
        } else {
            ++iter;
        }
    }
}

void VulkanFBOCache::reset() noexcept {
    m_framebuffer.clear();
    m_render_pass_ref_counts.clear();
    m_render_passes.clear();
}

}  // namespace render
}  // namespace coust
