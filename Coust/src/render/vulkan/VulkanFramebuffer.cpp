#include "pch.h"

#include "utils/math/Hash.h"
#include "render/vulkan/VulkanFramebuffer.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/VulkanRenderPass.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/utils/VulkanAllocation.h"

namespace coust {
namespace render {

VkDevice VulkanFramebuffer::get_device() const noexcept {
    return m_dev;
}

VkFramebuffer VulkanFramebuffer::get_handle() const noexcept {
    return m_handle;
}

VulkanFramebuffer::VulkanFramebuffer(VkDevice dev, Param const &param) noexcept
    : m_dev(dev), m_render_pass(param.render_pass) {
    COUST_ASSERT(param.render_pass, "");
    // all the attachment descriptions live here, color first, then resolve,
    // finally depth. At most 8 color attachment + 8 resolve attachment + depth
    // attachment Note: this array has the same order as the render pass it
    // attaches to
    std::array<VkImageView, MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1 + 1>
        attachments{};
    uint32_t attachment_idx = 0;
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        if (param.colors[i])
            attachments[attachment_idx++] = param.colors[i]->get_handle();
    }
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        if (param.resolves[i])
            attachments[attachment_idx++] = param.resolves[i]->get_handle();
    }
    if (param.depth)
        attachments[attachment_idx++] = param.depth->get_handle();
    if (param.depth_resolve)
        attachments[attachment_idx++] = param.depth_resolve->get_handle();
    VkFramebufferCreateInfo framebuffer_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = param.render_pass->get_handle(),
        .attachmentCount = attachment_idx,
        .pAttachments = attachments.data(),
        .width = param.width,
        .height = param.height,
        .layers = param.layers,
    };
    COUST_VK_CHECK(
        vkCreateFramebuffer(m_dev, &framebuffer_info, nullptr, &m_handle), "");
}

VulkanFramebuffer::VulkanFramebuffer(VulkanFramebuffer &&other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_render_pass(other.m_render_pass) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanFramebuffer &VulkanFramebuffer::operator=(
    VulkanFramebuffer &&other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    std::swap(m_render_pass, other.m_render_pass);
    return *this;
}

VulkanFramebuffer::~VulkanFramebuffer() noexcept {
    if (m_handle) {
        vkDestroyFramebuffer(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VulkanRenderPass const &VulkanFramebuffer::get_render_pass() const noexcept {
    return *m_render_pass;
}

static_assert(detail::IsVulkanResource<VulkanFramebuffer>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::VulkanFramebuffer::Param>::operator()(
    coust::render::VulkanFramebuffer::Param const &key) const noexcept {
    size_t h = coust::calc_std_hash(key.width);
    coust::hash_combine(h, key.height);
    coust::hash_combine(h, key.layers);
    for (uint32_t i = 0; i < key.colors.size(); ++i) {
        if (key.colors[i])
            coust::hash_combine(h, key.colors[i]->get_handle());
        else
            coust::hash_combine(h, 0);
    }
    for (uint32_t i = 0; i < key.resolves.size(); ++i) {
        if (key.resolves[i])
            coust::hash_combine(h, key.resolves[i]->get_handle());
        else
            coust::hash_combine(h, 0);
    }
    if (key.depth)
        coust::hash_combine(h, key.depth->get_handle());
    if (key.depth_resolve)
        coust::hash_combine(h, key.depth_resolve->get_handle());
    coust::hash_combine(h, key.render_pass->get_handle());
    return h;
}

bool equal_to<coust::render::VulkanFramebuffer::Param>::operator()(
    coust::render::VulkanFramebuffer::Param const &left,
    coust::render::VulkanFramebuffer::Param const &right) const noexcept {
    return left.width == right.width && left.height == right.height &&
           left.layers == right.layers &&
           std::ranges::equal(left.colors, right.colors) &&
           std::ranges::equal(left.resolves, right.resolves) &&
           left.depth == right.depth &&
           left.depth_resolve == right.depth_resolve &&
           left.render_pass->get_handle() == right.render_pass->get_handle();
}

}  // namespace std
