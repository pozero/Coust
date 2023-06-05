#pragma once

#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanAttachment.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/VulkanSwapchain.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanAttachment {
public:
    VulkanAttachment(
        VulkanImage& image, uint32_t level, uint32_t layer) noexcept;

    VulkanAttachment() noexcept = default;

    VulkanAttachment(VulkanAttachment&&) noexcept = default;

    VulkanAttachment(VulkanAttachment const&) noexcept = default;

    VulkanAttachment& operator=(VulkanAttachment&&) noexcept = default;

    VulkanAttachment& operator=(VulkanAttachment const&) noexcept = default;

    VkImageSubresourceRange get_subresource_range(
        VkImageAspectFlags aspect) const noexcept;

    const VulkanImageView* get_image_view(
        VkImageAspectFlags aspect) const noexcept;

    VkImageLayout get_layout() const noexcept;

public:
    VulkanImage* m_image = nullptr;
    uint32_t m_level = ~(0u);
    uint32_t m_layer = ~(0u);
};

class VulkanRenderTarget {
public:
    VulkanRenderTarget(VulkanRenderTarget&&) = delete;
    VulkanRenderTarget(VulkanRenderTarget const&) = delete;
    VulkanRenderTarget& operator=(VulkanRenderTarget&&) = delete;
    VulkanRenderTarget& operator=(VulkanRenderTarget const&) = delete;

public:
    // Render pass before present
    VulkanRenderTarget(VkDevice dev, VmaAllocator alloc, VkCommandBuffer cmdbuf,
        VkExtent2D extent, VkSampleCountFlagBits samples,
        std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> const& color,
        VulkanAttachment const& depth) noexcept;

    // Presenting render pass, it'll be attached to swapchain and have only one
    // color attachment
    explicit VulkanRenderTarget() noexcept;

    VulkanAttachment get_color(uint32_t idx) const noexcept;

    VkFormat get_color_format(uint32_t idx) const noexcept;

    VulkanAttachment get_color_msaa(uint32_t idx) const noexcept;

    VulkanAttachment get_depth() const noexcept;

    VkFormat get_depth_format() const noexcept;

    VulkanAttachment get_depth_msaa() const noexcept;

    VkExtent2D get_extent() const noexcept;

    VkSampleCountFlagBits get_sample_count() const noexcept;

    bool is_attached_to_swapchain() const noexcept;

    void attach(VulkanSwapchain& swapchain) noexcept;

private:
    std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> m_color{};

    std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> m_color_msaa{};

    VulkanAttachment m_depth{};

    VulkanAttachment m_depth_msaa{};

    VkExtent2D m_extent;

    VkSampleCountFlagBits m_sample_cnt = VK_SAMPLE_COUNT_1_BIT;

    bool m_attached_to_swapchian;
};

}  // namespace render
}  // namespace coust
