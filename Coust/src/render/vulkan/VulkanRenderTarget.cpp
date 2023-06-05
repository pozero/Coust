#include "pch.h"

#include "render/vulkan/VulkanRenderTarget.h"

namespace coust {
namespace render {

VulkanAttachment::VulkanAttachment(
    VulkanImage& image, uint32_t level, uint32_t layer) noexcept
    : m_image(&image), m_level(level), m_layer(layer) {
}

VkImageSubresourceRange VulkanAttachment::get_subresource_range(
    VkImageAspectFlags aspect) const noexcept {
    uint32_t level_cnt = 1;
    uint32_t layer_cnt = 1;
    if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        auto const range = m_image->get_primary_subrange();
        level_cnt = range.levelCount;
        layer_cnt = range.layerCount;
    }
    return VkImageSubresourceRange{
        .aspectMask = aspect,
        .baseMipLevel = m_level,
        .levelCount = level_cnt,
        .baseArrayLayer = m_layer,
        .layerCount = layer_cnt,
    };
}

const VulkanImageView* VulkanAttachment::get_image_view(
    VkImageAspectFlags aspect) const noexcept {
    return &m_image->get_view(get_subresource_range(aspect));
}

VkImageLayout VulkanAttachment::get_layout() const noexcept {
    return m_image ? m_image->get_layout(m_layer, m_level) :
                     VK_IMAGE_LAYOUT_UNDEFINED;
}

VulkanRenderTarget::VulkanRenderTarget(VkDevice dev, VmaAllocator alloc,
    VkCommandBuffer cmdbuf, VkExtent2D extent, VkSampleCountFlagBits samples,
    std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> const& color,
    VulkanAttachment const& depth) noexcept
    : m_color(color),
      m_depth(depth),
      m_extent(extent),
      m_sample_cnt(samples),
      m_attached_to_swapchian(false) {
    if (samples == VK_SAMPLE_COUNT_1_BIT)
        return;
    std::array<uint32_t, 3> constexpr related_queues{
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
    };
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        auto& attachment = m_color[i];
        if (!attachment.m_image)
            continue;
        if (attachment.m_image->m_msaa_accessory)
            m_color_msaa[i].m_image =
                attachment.m_image->m_msaa_accessory.get();
        else {
            // if the sample count of the color attachment is one, then we need
            // to create a msaa counterpart to resolve to the color attachment
            if (attachment.m_image->get_sample_count() ==
                VK_SAMPLE_COUNT_1_BIT) {
                auto msaa_accessory = memory::allocate_shared<VulkanImage>(
                    get_default_alloc(), dev, alloc, cmdbuf,
                    attachment.m_image->get_extent().width,
                    attachment.m_image->get_extent().height,
                    attachment.m_image->get_format(),
                    VulkanImage::Usage::color_attachment, 0, 0, related_queues,
                    1, m_sample_cnt, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_LAYOUT_UNDEFINED);
                attachment.m_image->m_msaa_accessory = msaa_accessory;
                m_color_msaa[i].m_image = msaa_accessory.get();
            }
            // Or the sample count of the color attachment is already
            // multisampled, which means it doesn't require resolvement.
        }
    }
    if (!m_depth.m_image)
        return;
    if (m_depth.m_image->get_sample_count() == VK_SAMPLE_COUNT_1_BIT) {
        auto msaa_accessory = memory::allocate_shared<VulkanImage>(
            get_default_alloc(), dev, alloc, cmdbuf,
            m_depth.m_image->get_extent().width,
            m_depth.m_image->get_extent().height, m_depth.m_image->get_format(),
            VulkanImage::Usage::depth_stencil_attachment, 0, 0, related_queues,
            1, m_sample_cnt, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_LAYOUT_UNDEFINED);
        m_depth.m_image->m_msaa_accessory = msaa_accessory;
        m_depth_msaa.m_image = msaa_accessory.get();
    }
}

VulkanRenderTarget::VulkanRenderTarget() noexcept
    : m_attached_to_swapchian(true) {
}

VulkanAttachment VulkanRenderTarget::get_color(uint32_t idx) const noexcept {
    return m_color[idx];
}

VkFormat VulkanRenderTarget::get_color_format(uint32_t idx) const noexcept {
    if (!m_color[idx].m_image) {
        return VK_FORMAT_UNDEFINED;
    } else {
        if (m_sample_cnt == VK_SAMPLE_COUNT_1_BIT) {
            return m_color[idx].m_image->get_format();
        } else {
            return m_color[idx].m_image->m_msaa_accessory->get_format();
        }
    }
}

VulkanAttachment VulkanRenderTarget::get_color_msaa(
    uint32_t idx) const noexcept {
    return m_color_msaa[idx];
}

VulkanAttachment VulkanRenderTarget::get_depth() const noexcept {
    return m_depth;
}

VkFormat VulkanRenderTarget::get_depth_format() const noexcept {
    if (!m_depth.m_image) {
        return VK_FORMAT_UNDEFINED;
    } else {
        if (m_sample_cnt == VK_SAMPLE_COUNT_1_BIT) {
            return m_depth.m_image->get_format();
        } else {
            return m_depth.m_image->m_msaa_accessory->get_format();
        }
    }
}

VulkanAttachment VulkanRenderTarget::get_depth_msaa() const noexcept {
    return m_depth_msaa;
}

VkExtent2D VulkanRenderTarget::get_extent() const noexcept {
    return m_extent;
}

VkSampleCountFlagBits VulkanRenderTarget::get_sample_count() const noexcept {
    return m_sample_cnt;
}

bool VulkanRenderTarget::is_attached_to_swapchain() const noexcept {
    return m_attached_to_swapchian;
}

void VulkanRenderTarget::attach(VulkanSwapchain& swapchain) noexcept {
    COUST_ASSERT(m_attached_to_swapchian, "");
    m_color.at(0).m_image = &swapchain.get_color_attachment();
    m_extent = swapchain.m_extent;
}

}  // namespace render
}  // namespace coust
