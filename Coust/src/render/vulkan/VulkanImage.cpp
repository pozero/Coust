#include "pch.h"

#include "utils/math/Hash.h"
#include "render/vulkan/VulkanImage.h"
#include "render/vulkan/VulkanStagePool.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanFormat.h"

namespace coust {
namespace render {

constexpr void get_image_config(VulkanImage::Usage usage,
    VkImageCreateInfo &image_info, VmaAllocationCreateInfo &allocation_info,
    VkImageViewType &view_type, VkImageLayout &default_layout,
    bool blitable) noexcept {
    // For the convenience of copying data between image (debug for example),
    // the image can be blitable
    VkImageUsageFlags const blit =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (blitable)
        image_info.usage |= blit;
    // https://gpuopen.com/learn/vulkan-renderpasses/
    // Finally, Vulkan includes the concept of transient attachments.
    // These are framebuffer attachments that begin in an uninitialized or
    // cleared state at the beginning of a renderpass, are written by one or
    // more subpasses, consumed by one or more subpasses and are ultimately
    // discarded at the end of the renderpass. In this scenario, the data in the
    // attachments only lives within the renderpass and never needs to be
    // written to main memory. Although we'll still allocate memory for such an
    // attachment, the data may never leave the GPU, instead only ever living in
    // cache. This saves bandwidth, reduces latency and improves power
    // efficiency.
    if (image_info.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        allocation_info.preferredFlags |=
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }
    switch (usage) {
        case VulkanImage::Usage::cube_map:
            image_info.arrayLayers = 6;
            image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            image_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            view_type = VK_IMAGE_VIEW_TYPE_CUBE;
            default_layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
            return;
        case VulkanImage::Usage::texture_2d:
            image_info.arrayLayers = 1;
            image_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            view_type = VK_IMAGE_VIEW_TYPE_2D;
            default_layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
            return;
        case VulkanImage::Usage::depth_stencil_attachment:
            image_info.arrayLayers = 1;
            image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            view_type = VK_IMAGE_VIEW_TYPE_2D;
            default_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            return;
        case VulkanImage::Usage::color_attachment:
            image_info.arrayLayers = 1;
            image_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            view_type = VK_IMAGE_VIEW_TYPE_2D;
            default_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            return;
        case VulkanImage::Usage::input_attachment:
            image_info.arrayLayers = 1;
            // Commonly, input attachment is also the color attachment for the
            // previous subpass
            image_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            image_info.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            view_type = VK_IMAGE_VIEW_TYPE_2D;
            default_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            return;
    }
    ASSUME(0);
}

VkDevice VulkanImageView::get_device() const noexcept {
    return m_dev;
}

VkImageView VulkanImageView::get_handle() const noexcept {
    return m_handle;
}

VulkanImageView::VulkanImageView(VkDevice dev, class VulkanImage *image,
    VkImageViewType type, VkImageSubresourceRange sub_range) noexcept
    : m_dev(dev) {
    COUST_ASSERT(sub_range.layerCount > 0 && sub_range.levelCount > 0, "");
    VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = image->get_handle(),
        .viewType = type,
        .format = image->get_format(),
        .components =
            {
                         .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                         },
        .subresourceRange = sub_range,
    };
    COUST_VK_CHECK(vkCreateImageView(m_dev, &view_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "");
}

VulkanImageView::~VulkanImageView() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyImageView(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkDevice VulkanImage::get_device() const noexcept {
    return m_dev;
}

VkImage VulkanImage::get_handle() const noexcept {
    return m_handle;
}

VulkanImage::VulkanImage(VkDevice dev, VmaAllocator alloc,
    VkCommandBuffer cmdbuf, uint32_t width, uint32_t height, VkFormat format,
    Usage usage, VkImageUsageFlags vk_usage_flags,
    VkImageCreateFlags vk_create_flags,
    std::array<uint32_t, 3> const &related_queues, uint32_t mip_levels,
    VkSampleCountFlagBits sample_cnt, VkImageTiling tiling,
    VkImageLayout initial_layout) noexcept
    : m_dev(dev),
      m_allocator(alloc),
      m_extent{width, height},
      m_format(format),
      m_sample_count(sample_cnt),
      m_mip_level_count(mip_levels) {
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = vk_create_flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mip_levels,
        .samples = sample_cnt,
        .tiling = tiling,
        .usage = vk_usage_flags,
        .initialLayout = initial_layout,
    };
    memory::vector<uint32_t, DefaultAlloc> all_queues{
        related_queues.begin(), related_queues.end(), get_default_alloc()};
    auto [erase_begin, erase_end] = std::ranges::unique(all_queues);
    all_queues.erase(erase_begin, erase_end);
    if (all_queues.size() > 1) {
        image_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        image_info.queueFamilyIndexCount = (uint32_t) all_queues.size();
        image_info.pQueueFamilyIndices = all_queues.data();
    }
    VmaAllocationCreateInfo allocation_create_info{};
    get_image_config(usage, image_info, allocation_create_info, m_view_type,
        m_default_layout, true);
    m_primary_subrange.aspectMask = is_depth_stencil_format(format) ?
                                        VK_IMAGE_ASPECT_DEPTH_BIT :
                                        VK_IMAGE_ASPECT_COLOR_BIT;
    m_primary_subrange.baseArrayLayer = 0;
    m_primary_subrange.layerCount = image_info.arrayLayers;
    m_primary_subrange.baseMipLevel = 0;
    m_primary_subrange.levelCount = image_info.mipLevels;
    VmaAllocationInfo ret_alloc_info{};
    COUST_VK_CHECK(
        vmaCreateImage(m_allocator, &image_info, &allocation_create_info,
            &m_handle, &m_allocation, &ret_alloc_info),
        "");
    get_view(m_primary_subrange);
    // the layout transition for texture is deferred until upload
    if (usage == Usage::color_attachment ||
        usage == Usage::depth_stencil_attachment ||
        usage == Usage::input_attachment)
        transition_layout(cmdbuf, m_default_layout, m_primary_subrange);
}

VulkanImage::VulkanImage(VkImage handle, uint32_t width, uint32_t height,
    VkFormat format, VkSampleCountFlagBits samples) noexcept
    : m_handle(handle),
      m_extent{width, height},
      m_format(format),
      m_sample_count(samples),
      m_mip_level_count(0) {
}

VulkanImage::~VulkanImage() noexcept {
    if (m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_allocator, m_handle, m_allocation);
    }
}

void VulkanImage::update(VulkanStagePool *stage_pool, VkCommandBuffer cmdbuf,
    VkFormat format, uint32_t width, uint32_t height, const void *data,
    uint32_t dst_layer, uint32_t dst_layer_cnt, uint32_t dst_level) noexcept {
    VkFormat const linear_dst_format = unpack_srgb_format(m_format);
    size_t const data_size =
        width * height * get_byte_per_pixel_from_format(format);
    std::span<const uint8_t> const data_span{(const uint8_t *) data, data_size};
    VkImageAspectFlags const aspect = is_depth_stencil_format(m_format);
    bool const need_format_conversion =
        format != VK_FORMAT_UNDEFINED && format != linear_dst_format;
    bool const need_resizing =
        width != m_extent.width || height != m_extent.height;
    if (need_resizing || need_format_conversion) {
        auto staging_img =
            stage_pool->acquire_staging_img(cmdbuf, format, width, height);
        staging_img->update(data_span);
        std::array<VkOffset3D, 2> const src_rect{
            VkOffset3D{              0,                0, 0},
            VkOffset3D{(int32_t) width, (int32_t) height, 1},
        };
        std::array<VkOffset3D, 2> const dst_rect{
            VkOffset3D{                       0,                         0, 0},
            VkOffset3D{(int32_t) m_extent.width, (int32_t) m_extent.height, 1},
        };
        VkImageBlit blit_info{
            .srcSubresource =
                {
                                 .aspectMask = aspect,
                                 .mipLevel = 0,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1,
                                 },
            .srcOffsets = {src_rect[0], src_rect[1]},
            .dstSubresource =
                {
                                 .aspectMask = aspect,
                                 .mipLevel = dst_level,
                                 .baseArrayLayer = dst_layer,
                                 .layerCount = dst_layer_cnt,
                                 },
            .dstOffsets = {dst_rect[0], dst_rect[1]},
        };
        VkImageSubresourceRange range{
            .aspectMask = aspect,
            .baseMipLevel = dst_level,
            .levelCount = 1,
            .baseArrayLayer = dst_layer,
            .layerCount = dst_layer_cnt,
        };
        transition_layout(cmdbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        vkCmdBlitImage(cmdbuf, staging_img->get_handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_info,
            VK_FILTER_NEAREST);
        transition_layout(cmdbuf, m_default_layout, range);
    } else {
        auto staging_buf = stage_pool->acquire_staging_buf(data_size);
        staging_buf->update(stage_pool, cmdbuf, data_span);
        VkBufferImageCopy copyInfo{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                   .mipLevel = dst_level,
                                   .baseArrayLayer = dst_layer,
                                   .layerCount = dst_layer_cnt,
                                   },
            .imageOffset = {                                      0,      0,    0 },
            .imageExtent = {                                  width, height,    1 },
        };
        VkImageSubresourceRange range{
            .aspectMask = aspect,
            .baseMipLevel = dst_level,
            .levelCount = 1,
            .baseArrayLayer = dst_layer,
            .layerCount = dst_layer_cnt,
        };
        transition_layout(cmdbuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        vkCmdCopyBufferToImage(cmdbuf, staging_buf->get_handle(), m_handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);
        transition_layout(cmdbuf, m_default_layout, range);
    }
}

void VulkanImage::transition_layout(VkCommandBuffer cmdbuf,
    VkImageLayout new_layout, VkImageSubresourceRange subrange) noexcept {
    const uint32_t layer_first = subrange.baseArrayLayer;
    const uint32_t layer_last =
        subrange.baseArrayLayer + subrange.layerCount - 1;
    const uint32_t level_first = subrange.baseMipLevel;
    const uint32_t level_last = subrange.baseMipLevel + subrange.levelCount - 1;
    const VkImageLayout oldLayout = get_layout(layer_first, level_first);
    // Check if the layout in the `subrange` is consistent
    if (oldLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
        for (uint32_t layer = layer_first; layer <= layer_last; ++layer) {
            for (uint32_t level = level_first; level <= level_last; ++level) {
                COUST_PANIC_IF(get_layout(layer, level) != oldLayout,
                    "Try to transition image subresource with "
                    "inconsisitent image layouts");
            }
        }
    }
    transition_image_layout(cmdbuf, image_blit_transition(VkImageMemoryBarrier2{
                                        .oldLayout = oldLayout,
                                        .newLayout = new_layout,
                                        .image = m_handle,
                                        .subresourceRange = subrange,
                                    }));
    // clear the old layout for undefined new layout
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        for (uint32_t layer = layer_first; layer <= layer_last; ++layer) {
            auto iterFirst =
                m_subrange_layouts.lower_bound((layer << 16) | level_first);
            auto iterLast =
                m_subrange_layouts.upper_bound((layer << 16) | level_last);
            m_subrange_layouts.erase(iterFirst, iterLast);
        }
    }
    // change the old layout to the new layout
    else {
        for (uint32_t layer = layer_first; layer <= layer_last; ++layer) {
            for (uint32_t level = level_first; level <= level_last; ++level) {
                m_subrange_layouts[(layer << 16) | level] = new_layout;
            }
        }
    }
}

void VulkanImage::change_layout(
    uint32_t layer, uint32_t level, VkImageLayout layout) noexcept {
    const uint32_t key = (layer << 16) | level;
    if (layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        if (auto iter = m_subrange_layouts.find(key);
            iter != m_subrange_layouts.end())
            m_subrange_layouts.erase(iter);
    } else {
        m_subrange_layouts[key] = layout;
    }
}

VulkanImageView const &VulkanImage::get_view(
    VkImageSubresourceRange subrange) noexcept {
    if (auto iter = m_image_views.find(subrange); iter != m_image_views.end()) {
        return iter->second;
    }
    VulkanImageView view{m_dev, this, m_view_type, subrange};
    m_image_views.emplace(subrange, std::move(view));
    return m_image_views.at(subrange);
}

VulkanImageView const &VulkanImage::get_sinigle_layer_view(
    VkImageAspectFlags aspect, uint32_t layer, uint32_t level) noexcept {
    return get_view(VkImageSubresourceRange{
        .aspectMask = aspect,
        .baseMipLevel = level,
        .levelCount = 1,
        .baseArrayLayer = layer,
        .layerCount = 1,
    });
}

void VulkanImage::set_primary_subrange(
    uint32_t min_level, uint32_t max_level) noexcept {
    max_level = std::min(max_level, m_mip_level_count - 1);
    m_primary_subrange.baseMipLevel = min_level;
    m_primary_subrange.levelCount = max_level - min_level + 1;
    get_view(m_primary_subrange);
}

VkImageLayout VulkanImage::get_primary_layout() const noexcept {
    return get_layout(
        m_primary_subrange.baseArrayLayer, m_primary_subrange.baseMipLevel);
}

VulkanImageView const &VulkanImage::get_primary_view() const noexcept {
    return m_image_views.at(m_primary_subrange);
}

VkImageSubresourceRange VulkanImage::get_primary_subrange() const noexcept {
    return m_primary_subrange;
}

VkImageLayout VulkanImage::get_layout(
    uint32_t layer, uint32_t level) const noexcept {
    const uint32_t key = (layer << 16) | level;
    auto iter = m_subrange_layouts.find(key);
    if (iter != m_subrange_layouts.end())
        return iter->second;
    else
        return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkFormat VulkanImage::get_format() const noexcept {
    return m_format;
}

VkSampleCountFlagBits VulkanImage::get_sample_count() const noexcept {
    return m_sample_count;
}

VkExtent2D VulkanImage::get_extent() const noexcept {
    return m_extent;
}

uint32_t VulkanImage::get_mip_level() const noexcept {
    return m_mip_level_count;
}

VkDevice VulkanHostImage::get_device() const noexcept {
    return m_dev;
}

VkImage VulkanHostImage::get_handle() const noexcept {
    return m_handle;
}

VulkanHostImage::VulkanHostImage(VkDevice dev, VmaAllocator alloc,
    VkCommandBuffer cmdbuf, VkFormat format, uint32_t width,
    uint32_t height) noexcept
    : m_dev(dev),
      m_allocator(alloc),
      m_extent{width, height},
      m_format(format) {
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocation_create_info{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };
    VmaAllocationInfo ret_alloc_info{};
    COUST_VK_CHECK(
        vmaCreateImage(m_allocator, &image_info, &allocation_create_info,
            &m_handle, &m_allocation, &ret_alloc_info),
        "");
    m_mapped = (uint8_t *) ret_alloc_info.pMappedData;
    transition_image_layout(
        cmdbuf, image_blit_transition(VkImageMemoryBarrier2{
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .image = m_handle,
                    .subresourceRange =
                        {
                                           .aspectMask = get_aspect(),
                                           .baseMipLevel = 0,
                                           .levelCount = 1,
                                           .baseArrayLayer = 0,
                                           .layerCount = 1,
                                           },
    }));
}

VulkanHostImage::~VulkanHostImage() noexcept {
    if (m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_allocator, m_handle, m_allocation);
    }
}

void VulkanHostImage::update(std::span<const uint8_t> data) noexcept {
    std::copy(data.begin(), data.end(), m_mapped);
    vmaFlushAllocation(m_allocator, m_allocation, 0, data.size());
}

VkExtent2D VulkanHostImage::get_extent() const noexcept {
    return m_extent;
}

VkFormat VulkanHostImage::get_format() const noexcept {
    return m_format;
}

VkImageAspectFlags VulkanHostImage::get_aspect() const noexcept {
    return is_depth_stencil_format(m_format) ? VK_IMAGE_ASPECT_DEPTH_BIT :
                                               VK_IMAGE_ASPECT_COLOR_BIT;
}

static_assert(detail::IsVulkanResource<VulkanImageView>);
static_assert(detail::IsVulkanResource<VulkanImage>);
static_assert(detail::IsVulkanResource<VulkanHostImage>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<VkImageSubresourceRange>::operator()(
    VkImageSubresourceRange const &key) const noexcept {
    size_t h = coust::calc_std_hash(key.aspectMask);
    coust::hash_combine(h, key.baseMipLevel);
    coust::hash_combine(h, key.levelCount);
    coust::hash_combine(h, key.baseArrayLayer);
    coust::hash_combine(h, key.layerCount);
    return h;
}

bool equal_to<VkImageSubresourceRange>::operator()(
    VkImageSubresourceRange const &left,
    VkImageSubresourceRange const &right) const noexcept {
    return left.aspectMask == right.aspectMask &&
           left.baseMipLevel == right.baseMipLevel &&
           left.levelCount == right.levelCount &&
           left.baseArrayLayer == right.baseArrayLayer &&
           left.layerCount == right.layerCount;
}

}  // namespace std
