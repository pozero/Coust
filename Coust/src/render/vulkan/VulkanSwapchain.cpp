#include "pch.h"

#include "core/Application.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/VulkanSwapchain.h"

namespace coust {
namespace render {

VkDevice VulkanSwapchain::get_device() const noexcept {
    return m_dev;
}

VkSwapchainKHR VulkanSwapchain::get_handle() const noexcept {
    return m_handle;
}

VulkanSwapchain::VulkanSwapchain(VkDevice dev, VkPhysicalDevice phy_dev,
    VkSurfaceKHR surface, uint32_t graphics_queue_idx,
    uint32_t present_queue_idx, VulkanCommandBufferCache& cmd_cache) noexcept
    : m_dev(dev),
      m_phy_dev(phy_dev),
      m_surface(surface),
      m_graphics_queue_idx(graphics_queue_idx),
      m_present_queue_idx(present_queue_idx),
      m_cmd_cache(cmd_cache) {
}

void VulkanSwapchain::prepare() noexcept {
    {
        VkSurfaceFormatKHR best_surface_format{};
        uint32_t surface_format_cnt = 0;
        memory::vector<VkSurfaceFormatKHR, DefaultAlloc> surface_formats{
            get_default_alloc()};
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            m_phy_dev, m_surface, &surface_format_cnt, nullptr);
        surface_formats.resize(surface_format_cnt);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            m_phy_dev, m_surface, &surface_format_cnt, surface_formats.data());
        best_surface_format = surface_formats[0];
        for (auto const& format : surface_formats) {
            if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                best_surface_format = format;
                break;
            } else if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                best_surface_format = format;
            }
        }
        m_surface_format = best_surface_format;
    }
    {
        VkPresentModeKHR best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        uint32_t present_mode_cnt = 0;
        memory::vector<VkPresentModeKHR, DefaultAlloc> present_modes{
            get_default_alloc()};
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            m_phy_dev, m_surface, &present_mode_cnt, nullptr);
        present_modes.resize(present_mode_cnt);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            m_phy_dev, m_surface, &present_mode_cnt, present_modes.data());
        for (auto const mode : present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                best_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
        m_present_mode = best_present_mode;
    }
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_phy_dev, m_surface, &capabilities);
        uint32_t img_cnt = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount != 0 &&
            img_cnt > capabilities.maxImageCount)
            img_cnt = capabilities.maxImageCount;
        m_min_image_count = img_cnt;
    }
    {
        VkFormat best_depth_format = VK_FORMAT_UNDEFINED;
        std::array candidates{VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
        for (uint32_t i = 0; i < candidates.size(); ++i) {
            VkFormatProperties props{};
            vkGetPhysicalDeviceFormatProperties(
                m_phy_dev, candidates[i], &props);
            if (props.optimalTilingFeatures &
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                best_depth_format = candidates[i];
                break;
            }
        }
        COUST_PANIC_IF(best_depth_format == VK_FORMAT_UNDEFINED, "");
        m_depth_format = best_depth_format;
    }
}

bool VulkanSwapchain::create() noexcept {
    auto [width, height] =
        Application::get_instance().get_window().get_drawable_size();
    while (width == 0 || height == 0) {
        std::tie(width, height) =
            Application::get_instance().get_window().get_drawable_size();
        Application::get_instance().get_window().poll_events();
    }
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_phy_dev, m_surface, &capabilities);
    if (capabilities.currentExtent.width == (uint32_t) -1 ||
        capabilities.currentExtent.height == (uint32_t) -1) {
        std::tie(width, height) =
            Application::get_instance().get_window().get_drawable_size();
        m_extent = VkExtent2D{
            std::clamp((uint32_t) width, capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width),
            std::clamp((uint32_t) height, capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height)};
    } else {
        m_extent = capabilities.currentExtent;
    }
    VkSwapchainCreateInfoKHR swapchain_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = m_min_image_count,
        .imageFormat = m_surface_format.format,
        .imageExtent = m_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = m_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    std::array const queue_family_indices{
        m_graphics_queue_idx, m_present_queue_idx};
    if (m_graphics_queue_idx != m_present_queue_idx) {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount =
            (uint32_t) queue_family_indices.size();
        swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    COUST_VK_CHECK(vkCreateSwapchainKHR(m_dev, &swapchain_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "");
    uint32_t img_cnt = 0;
    memory::vector<VkImage, DefaultAlloc> img_handles{get_default_alloc()};
    vkGetSwapchainImagesKHR(m_dev, m_handle, &img_cnt, nullptr);
    img_handles.resize(img_cnt);
    m_images.reserve(img_cnt);
    vkGetSwapchainImagesKHR(m_dev, m_handle, &img_cnt, img_handles.data());
    for (auto const handle : img_handles) {
        m_images.emplace_back(m_dev, handle, m_extent.width, m_extent.height,
            m_surface_format.format, VK_SAMPLE_COUNT_1_BIT);
    }
    VkSemaphoreCreateInfo sama_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0};
    COUST_VK_CHECK(vkCreateSemaphore(m_dev, &sama_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_img_avai_singal),
        "");
    m_is_next_img_acquired = false;
    return true;
}

void VulkanSwapchain::destroy() noexcept {
    m_cmd_cache.flush();
    m_cmd_cache.wait();
    m_images.clear();
    vkDestroySwapchainKHR(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    vkDestroySemaphore(m_dev, m_img_avai_singal, COUST_VULKAN_ALLOC_CALLBACK);
}

bool VulkanSwapchain::acquire() noexcept {
    VkResult res = vkAcquireNextImageKHR(m_dev, m_handle,
        std::numeric_limits<uint64_t>::max(), m_img_avai_singal, VK_NULL_HANDLE,
        &m_img_idx);
    COUST_PANIC_IF_NOT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR,
        "vkAcquireNextImageKHR return error: {}", to_string_view(res));
    m_cmd_cache.inject_dependency(m_img_avai_singal);
    m_is_next_img_acquired = true;
    if (res == VK_SUBOPTIMAL_KHR && !m_is_suboptimal) {
        COUST_WARN("Suboptimal swapchain image");
        m_is_suboptimal = true;
    }
    return true;
}

void VulkanSwapchain::make_presentable() noexcept {
    get_color_attachment().transition_layout(m_cmd_cache.get(),
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1});
}

VulkanImage& VulkanSwapchain::get_color_attachment() noexcept {
    return m_images[m_img_idx];
}

bool VulkanSwapchain::has_resized() const noexcept {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_phy_dev, m_surface, &capabilities);
    return capabilities.currentExtent.width != m_extent.width ||
           capabilities.currentExtent.height != m_extent.height;
}

uint32_t VulkanSwapchain::get_image_idx() const noexcept {
    return m_img_idx;
}

static_assert(detail::IsVulkanResource<VulkanSwapchain>);

}  // namespace render
}  // namespace coust
