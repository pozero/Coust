#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"
#include "render/vulkan/VulkanDriver.h"
#include "render/Renderer.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

Renderer::Renderer() noexcept {
    memory::vector<const char*, DefaultAlloc> inst_ext{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> inst_layer{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> dev_ext{get_default_alloc()};
    VkPhysicalDeviceFeatures required_phydev_features{};
    const void* inst_creation_pnext = nullptr;
    const void* dev_creation_pnext = nullptr;
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    dev_ext.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    VkPhysicalDeviceSynchronization2Features phydev_sync2_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
    };
    dev_creation_pnext = &phydev_sync2_feature;
#if defined(COUST_VK_DBG)
    inst_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    inst_layer.push_back("VK_LAYER_KHRONOS_validation");
#endif
    m_vk_driver.initialize(inst_ext, inst_layer, dev_ext,
        required_phydev_features, inst_creation_pnext, dev_creation_pnext);
}

}  // namespace render
}  // namespace coust
