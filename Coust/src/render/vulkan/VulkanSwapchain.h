#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/Compiler.h"
#include "render/vulkan/VulkanImage.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanCommandBufferCache;

class VulkanSwapchain {
public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(VulkanSwapchain &&) = delete;
    VulkanSwapchain(VulkanSwapchain const &) = delete;
    VulkanSwapchain &operator=(VulkanSwapchain &&) = delete;
    VulkanSwapchain &operator=(VulkanSwapchain const &) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_SWAPCHAIN_KHR;

    VkDevice get_device() const noexcept;

    VkSwapchainKHR get_handle() const noexcept;

public:
    VulkanSwapchain(VkDevice dev, VkPhysicalDevice phy_dev,
        VkSurfaceKHR surface, uint32_t graphics_queue_idx,
        uint32_t present_queue_idx,
        VulkanCommandBufferCache &cmd_cache) noexcept;

    void prepare() noexcept;

    bool create() noexcept;

    void destroy() noexcept;

    bool acquire() noexcept;

    VulkanImage &get_color_attachment() noexcept;

    bool has_resized() const noexcept;

    uint32_t get_image_idx() const noexcept;

public:
    VkSurfaceFormatKHR m_surface_format;

    VkExtent2D m_extent;

    VkSemaphore m_img_avai_singal = VK_NULL_HANDLE;

    VkFormat m_depth_format = VK_FORMAT_UNDEFINED;

    VkPresentModeKHR m_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t m_min_image_count = 0;

    bool m_is_next_img_acquired = false;

    bool m_is_suboptimal = false;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkSwapchainKHR m_handle = VK_NULL_HANDLE;

    VkPhysicalDevice m_phy_dev = VK_NULL_HANDLE;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    uint32_t m_graphics_queue_idx = ~(0u);

    uint32_t m_present_queue_idx = ~(0u);

    VulkanCommandBufferCache &m_cmd_cache;

    memory::vector<VulkanImage, DefaultAlloc> m_images{get_default_alloc()};

    uint32_t m_img_idx = 0;
};

}  // namespace render
}  // namespace coust
