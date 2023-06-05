#pragma once

#include "utils/Compiler.h"
#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/SmartPtr.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
#include "vk_mem_alloc.h"
WARNING_POP

namespace std {

template <>
struct hash<VkImageSubresourceRange> {
    std::size_t operator()(VkImageSubresourceRange const& key) const noexcept;
};

template <>
struct equal_to<VkImageSubresourceRange> {
    bool operator()(VkImageSubresourceRange const& left,
        VkImageSubresourceRange const& right) const noexcept;
};

}  // namespace std

namespace coust {
namespace render {

class VulkanImageView {
public:
    VulkanImageView() = delete;
    VulkanImageView(VulkanImageView const&) = delete;
    VulkanImageView& operator=(VulkanImageView const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_IMAGE_VIEW;

    VkDevice get_device() const noexcept;

    VkImageView get_handle() const noexcept;

public:
    VulkanImageView(VkDevice dev, class VulkanImage* image,
        VkImageViewType type, VkImageSubresourceRange sub_range) noexcept;

    VulkanImageView(VulkanImageView&&) noexcept = default;

    VulkanImageView& operator=(VulkanImageView&&) noexcept = default;

    ~VulkanImageView() noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkImageView m_handle = VK_NULL_HANDLE;
};

class VulkanImage {
public:
    VulkanImage() = delete;
    VulkanImage(VulkanImage const&) = delete;
    VulkanImage& operator=(VulkanImage const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_IMAGE;

    VkDevice get_device() const noexcept;

    VkImage get_handle() const noexcept;

    enum class Usage {
        cube_map,
        texture_2d,
        depth_stencil_attachment,
        color_attachment,
        input_attachment,
    };

public:
    VulkanImage(VkDevice dev, VmaAllocator alloc, VkCommandBuffer cmdbuf,
        uint32_t width, uint32_t height, VkFormat format, Usage usage,
        VkImageUsageFlags vk_usage_flags, VkImageCreateFlags vk_create_flags,
        std::array<uint32_t, 3> const& related_queues, uint32_t mip_levels = 1,
        VkSampleCountFlagBits sample_cnt = VK_SAMPLE_COUNT_1_BIT,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED) noexcept;

    VulkanImage(VkImage handle, uint32_t width, uint32_t height,
        VkFormat format, VkSampleCountFlagBits samples) noexcept;

    VulkanImage(VulkanImage&&) noexcept = default;

    VulkanImage& operator=(VulkanImage&&) noexcept = default;

    ~VulkanImage() noexcept;

    void update(class VulkanStagePool& stage_pool, VkCommandBuffer cmdbuf,
        VkFormat format, uint32_t width, uint32_t height, const void* data,
        uint32_t dst_layer = 0, uint32_t dst_layer_cnt = 1,
        uint32_t dst_level = 0) noexcept;

    void transition_layout(VkCommandBuffer cmdbuf, VkImageLayout layout,
        VkImageSubresourceRange subrange) noexcept;

    // If the class is just a wrapper around a `VkImage` handle, like a
    // swapchain image, then its layout might be changed during renderpass. We
    // can use this method to keep track of the actual layout
    void change_layout(
        uint32_t layer, uint32_t level, VkImageLayout layout) noexcept;

    VulkanImageView const& get_view(VkImageSubresourceRange subrange) noexcept;

    VulkanImageView const& get_sinigle_layer_view(
        VkImageAspectFlags aspect, uint32_t layer, uint32_t level) noexcept;

    void set_primary_subrange(uint32_t min_level, uint32_t max_level) noexcept;

    VkImageLayout get_primary_layout() const noexcept;

    VulkanImageView const& get_primary_view() const noexcept;

    VkImageSubresourceRange get_primary_subrange() const noexcept;

    VkImageLayout get_layout(uint32_t layer, uint32_t level) const noexcept;

    VkFormat get_format() const noexcept;

    VkSampleCountFlagBits get_sample_count() const noexcept;

    VkExtent2D get_extent() const noexcept;

    uint32_t get_mip_level() const noexcept;

public:
    memory::shared_ptr<VulkanImage> m_msaa_accessory{};

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkImage m_handle = VK_NULL_HANDLE;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VmaAllocation m_allocation = VK_NULL_HANDLE;

    memory::robin_map<VkImageSubresourceRange, VulkanImageView, DefaultAlloc>
        m_image_views{get_default_alloc()};

    memory::map<uint32_t, VkImageLayout, DefaultAlloc> m_subrange_layouts{
        get_default_alloc()};

    VkImageSubresourceRange m_primary_subrange;

    VkExtent2D m_extent;

    VkFormat m_format;

    VkImageLayout m_default_layout;

    VkImageViewType m_view_type;

    VkSampleCountFlagBits m_sample_count;

    uint32_t m_mip_level_count;
};

class VulkanHostImage {
public:
    VulkanHostImage() = delete;
    VulkanHostImage(VulkanHostImage const&) = delete;
    VulkanHostImage& operator=(VulkanHostImage const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_IMAGE;

    VkDevice get_device() const noexcept;

    VkImage get_handle() const noexcept;

public:
    VulkanHostImage(VkDevice dev, VmaAllocator alloc, VkCommandBuffer cmdbuf,
        VkFormat format, uint32_t width, uint32_t height) noexcept;

    VulkanHostImage(VulkanHostImage&&) noexcept = default;

    VulkanHostImage& operator=(VulkanHostImage&&) noexcept = default;

    ~VulkanHostImage() noexcept;

    void update(std::span<const uint8_t> data) noexcept;

    VkExtent2D get_extent() const noexcept;

    VkFormat get_format() const noexcept;

    VkImageAspectFlags get_aspect() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkImage m_handle = VK_NULL_HANDLE;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VmaAllocation m_allocation = VK_NULL_HANDLE;

    VkExtent2D m_extent;

    uint8_t* m_mapped = nullptr;

    VkFormat m_format;
};

}  // namespace render
}  // namespace coust
