#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "render/vulkan/utils/IncompleteVulkan.h"

VK_DEFINE_HANDLE(VmaAllocator)

namespace coust {
namespace render {

class VulkanDriver {
public:
    VulkanDriver() = delete;
    VulkanDriver(VulkanDriver&&) = delete;
    VulkanDriver(VulkanDriver const&) = delete;
    VulkanDriver& operator=(VulkanDriver&&) = delete;
    VulkanDriver& operator=(VulkanDriver const&) = delete;

public:
    VulkanDriver(memory::vector<const char*, DefaultAlloc>& instance_extension,
        memory::vector<const char*, DefaultAlloc>& instance_layer,
        memory::vector<const char*, DefaultAlloc>& device_extension,
        VkPhysicalDeviceFeatures const& required_phydev_feature,
        const void* instance_creation_pnext,
        const void* device_creation_pnext) noexcept;

    ~VulkanDriver() noexcept;

public:
    int32_t m_max_msaa_sample = 1;
    uint32_t m_graphics_queue_family_idx = std::numeric_limits<uint32_t>::max();
    uint32_t m_present_queue_family_idx = std::numeric_limits<uint32_t>::max();
    uint32_t m_compute_queue_family_idx = std::numeric_limits<uint32_t>::max();

private:
    VkInstance m_instance = VK_NULL_HANDLE;
#if defined(COUST_VK_DBG)
    VkDebugUtilsMessengerEXT m_dbg_messenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_phydev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkQueue m_compute_queue = VK_NULL_HANDLE;
    VmaAllocator m_vma_alloc = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace coust
