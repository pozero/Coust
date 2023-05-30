#pragma once

#include "utils/Compiler.h"

DISABLE_ALL_WARNING
// I think it's generally a bad idea to include <vulkan/vulkan.h>
// everywhere and here's some temperary fix
#ifndef VULKAN_CORE_H_
    #define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkDebugUtilsMessengerEXT)
VK_DEFINE_HANDLE(VkSurfaceKHR)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_HANDLE(VkQueue)
typedef struct VkPhysicalDeviceFeatures VkPhysicalDeviceFeatures;
    #define VK_MAKE_API_VERSION(variant, major, minor, patch)                  \
        ((((uint32_t) (variant)) << 29) | (((uint32_t) (major)) << 22) |       \
            (((uint32_t) (minor)) << 12) | ((uint32_t) (patch)))
    #define VK_API_VERSION_1_3                                                 \
        VK_MAKE_API_VERSION(                                                   \
            0, 1, 3, 0)  // Patch version should always be set to 0
    #define VK_NULL_HANDLE nullptr
#endif
