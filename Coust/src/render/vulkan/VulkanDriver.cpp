#include "pch.h"

#include "utils/Compiler.h"
#include "core/Application.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanVersion.h"
#include "render/vulkan/VulkanDriver.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
#include "vk_mem_alloc.h"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
inline VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    [[maybe_unused]] void* user_data) {
    memory::string<DefaultAlloc> message{
        callback_data->pMessage, get_default_alloc()};
    for (uint32_t i = 0; i < callback_data->objectCount; ++i) {
        if (callback_data->pObjects[i].pObjectName) {
            message += "\n\t";
            message += callback_data->pObjects[i].pObjectName;
            message += ": ";
            message += to_string_view(callback_data->pObjects[i].objectType);
        }
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        COUST_INFO(message);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        COUST_WARN(message);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        COUST_ERROR(message);
    }
    return VK_FALSE;
}
WARNING_POP

}  // namespace detail

VulkanDriver::VulkanDriver(
    memory::vector<const char*, DefaultAlloc>& instance_extension,
    memory::vector<const char*, DefaultAlloc>& instance_layer,
    memory::vector<const char*, DefaultAlloc>& device_extension,
    VkPhysicalDeviceFeatures const& required_phydev_feature,
    const void* instance_creation_pnext,
    const void* device_creation_pnext) noexcept {
    COUST_VK_CHECK(volkInitialize(), "Can't initialize volk");
    {
        VkApplicationInfo app_info{
            .pApplicationName = "Coust",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = COUST_VULKAN_API_VERSION,
        };
        VkInstanceCreateInfo instance_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
        };

        {
            uint32_t sdl_required_extension_cnt = 0;
            COUST_PANIC_IF_NOT(Application::get_instance()
                                   .get_window()
                                   .get_required_vkinstance_extension(
                                       &sdl_required_extension_cnt, nullptr),
                "Can't get the number of required vulkan instance extension "
                "needed by SDL");
            size_t const origin_ext_cnt = instance_extension.size();
            instance_extension.resize(
                origin_ext_cnt + sdl_required_extension_cnt);
            COUST_PANIC_IF_NOT(Application::get_instance()
                                   .get_window()
                                   .get_required_vkinstance_extension(
                                       &sdl_required_extension_cnt,
                                       &instance_extension[origin_ext_cnt]),
                "Can't get the names of required vulkan instance extension "
                "needed by SDL");
            uint32_t provided_extension_cnt = 0;
            COUST_VK_CHECK(vkEnumerateInstanceExtensionProperties(
                               nullptr, &provided_extension_cnt, nullptr),
                "");
            memory::vector<VkExtensionProperties, DefaultAlloc> ext_props{
                provided_extension_cnt, {}, get_default_alloc()};
            COUST_VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr,
                               &provided_extension_cnt, ext_props.data()),
                "");
            for (auto const required_ext : instance_extension) {
                COUST_PANIC_IF_NOT(
                    std::ranges::any_of(ext_props,
                        [required_ext](VkExtensionProperties const& prop) {
                            return strcmp(prop.extensionName, required_ext) ==
                                   0;
                        }),
                    "Required vulkan instance extension {} isn't supported",
                    required_ext);
            }
            instance_info.enabledExtensionCount =
                (uint32_t) instance_extension.size();
            instance_info.ppEnabledExtensionNames = instance_extension.data();
        }

        {
            uint32_t provided_layer_cnt = 0;
            COUST_VK_CHECK(vkEnumerateInstanceLayerProperties(
                               &provided_layer_cnt, nullptr),
                "");
            memory::vector<VkLayerProperties, DefaultAlloc> provided_layers{
                provided_layer_cnt, {}, get_default_alloc()};
            COUST_VK_CHECK(vkEnumerateInstanceLayerProperties(
                               &provided_layer_cnt, provided_layers.data()),
                "");
            for (auto const required_layer : instance_layer) {
                COUST_PANIC_IF_NOT(
                    std::ranges::any_of(provided_layers,
                        [required_layer](VkLayerProperties const& prop) {
                            return strcmp(prop.layerName, required_layer) == 0;
                        }),
                    "Required vulkan layer {} isn't supported", required_layer);
            }
            instance_info.enabledLayerCount = (uint32_t) instance_layer.size();
            instance_info.ppEnabledLayerNames = instance_layer.data();
        }

#if defined(COUST_VK_DBG)
        // extra vulkan debug utils messenger for instance creation &
        // destruction
        VkDebugUtilsMessengerCreateInfoEXT dgb_messenger_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = instance_creation_pnext,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = detail::vulkan_debug_messenger_callback,
            .pUserData = nullptr,
        };
        instance_info.pNext = &dgb_messenger_info;
#else
        instance_info.pNext = instance_creation_pnext;
#endif

        COUST_VK_CHECK(vkCreateInstance(&instance_info,
                           COUST_VULKAN_ALLOC_CALLBACK, &m_instance),
            "Can't create vulkan instance");
        volkLoadInstance(m_instance);
    }

#if defined(COUST_VK_DBG)
    {
        VkDebugUtilsMessengerCreateInfoEXT dgb_messenger_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = detail::vulkan_debug_messenger_callback,
            .pUserData = nullptr,
        };
        COUST_VK_CHECK(
            vkCreateDebugUtilsMessengerEXT(m_instance, &dgb_messenger_info,
                COUST_VULKAN_ALLOC_CALLBACK, &m_dbg_messenger),
            "Can't create vulkan debug utils messenger");
    }
#endif

    COUST_PANIC_IF_NOT(
        Application::get_instance().get_window().create_vksurface(
            m_instance, &m_surface),
        "Can't create vulkan surface from SDL");

    {
        uint32_t physical_device_cnt = 0;
        COUST_VK_CHECK(vkEnumeratePhysicalDevices(
                           m_instance, &physical_device_cnt, nullptr),
            "");
        memory::vector<VkPhysicalDevice, DefaultAlloc> physical_devices{
            physical_device_cnt, nullptr, get_default_alloc()};
        COUST_VK_CHECK(vkEnumeratePhysicalDevices(m_instance,
                           &physical_device_cnt, physical_devices.data()),
            "");
        int highest_physical_device_score = -1;
        for (auto const& physical_dev : physical_devices) {
            std::optional<uint32_t> graphics_queue_family_idx{};
            std::optional<uint32_t> present_queue_family_idx{};
            std::optional<uint32_t> compute_queue_family_idx{};
            uint32_t device_queue_family_cnt = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                physical_dev, &device_queue_family_cnt, nullptr);
            memory::vector<VkQueueFamilyProperties, DefaultAlloc>
                queue_family_props{
                    device_queue_family_cnt, {}, get_default_alloc()};
            vkGetPhysicalDeviceQueueFamilyProperties(physical_dev,
                &device_queue_family_cnt, queue_family_props.data());
            for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
                VkBool32 support_present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    physical_dev, i, m_surface, &support_present);
                VkBool32 const support_graphics =
                    queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                if (support_present && support_graphics) {
                    graphics_queue_family_idx = i;
                    present_queue_family_idx = i;
                    break;
                }
                if (!graphics_queue_family_idx.has_value() &&
                    support_graphics) {
                    graphics_queue_family_idx = i;
                }
                if (!present_queue_family_idx.has_value() && support_present) {
                    present_queue_family_idx = i;
                }
            }
            for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
                bool const support_compute = (queue_family_props[i].queueFlags &
                                                 VK_QUEUE_COMPUTE_BIT) != 0;
                if (!support_compute)
                    continue;
                if (graphics_queue_family_idx.has_value() &&
                    graphics_queue_family_idx.value() != i) {
                    compute_queue_family_idx = i;
                    break;
                }
                if (!compute_queue_family_idx.has_value()) {
                    compute_queue_family_idx = i;
                }
            }
            if (!graphics_queue_family_idx.has_value() ||
                !present_queue_family_idx.has_value() ||
                !compute_queue_family_idx.has_value()) {
                continue;
            }

            bool support_extension = true;
            uint32_t device_extension_cnt = 0;
            COUST_VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_dev,
                               nullptr, &device_extension_cnt, nullptr),
                "");
            memory::vector<VkExtensionProperties, DefaultAlloc> dev_ext_props{
                device_extension_cnt, {}, get_default_alloc()};
            COUST_VK_CHECK(
                vkEnumerateDeviceExtensionProperties(physical_dev, nullptr,
                    &device_extension_cnt, dev_ext_props.data()),
                "");
            for (auto const required_ext : device_extension) {
                support_extension =
                    support_extension &&
                    std::ranges::any_of(dev_ext_props,
                        [required_ext](VkExtensionProperties const& prop) {
                            return strcmp(prop.extensionName, required_ext) ==
                                   0;
                        });
            }
            if (!support_extension)
                continue;

            {
                bool support_feature = true;
                VkPhysicalDeviceFeatures dev_features{};
                vkGetPhysicalDeviceFeatures(physical_dev, &dev_features);
                static_assert(
                    sizeof(VkPhysicalDeviceFeatures) % sizeof(VkBool32) == 0);
                size_t constexpr boolean_cnt =
                    sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
                std::span<const VkBool32, boolean_cnt> provided_features{
                    (const VkBool32*) &dev_features,
                    (const VkBool32*) ptr_math::add(
                        &dev_features, sizeof(VkPhysicalDeviceFeatures))};
                std::span<const VkBool32, boolean_cnt> required_features{
                    (const VkBool32*) &required_phydev_feature,
                    (const VkBool32*) ptr_math::add(&required_phydev_feature,
                        sizeof(VkPhysicalDeviceFeatures))};
                for (uint32_t i = 0; i < provided_features.size(); ++i) {
                    if (required_features[i] == VK_TRUE &&
                        provided_features[i] == VK_FALSE) {
                        support_feature = false;
                        break;
                    }
                }
                if (!support_feature)
                    continue;
            }

            VkPhysicalDeviceProperties dev_props{};
            vkGetPhysicalDeviceProperties(physical_dev, &dev_props);

            int score = 0;
            if (dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                score += 1000;
            else if (dev_props.deviceType ==
                     VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score += 100;
            }
            if (graphics_queue_family_idx.value() ==
                present_queue_family_idx.value()) {
                score += 50;
            }
            if (compute_queue_family_idx.value() !=
                graphics_queue_family_idx.value()) {
                score += 50;
            }
            if (score > highest_physical_device_score) {
                highest_physical_device_score = score;
                m_phydev = physical_dev;
                m_graphics_queue_family_idx = graphics_queue_family_idx.value();
                m_present_queue_family_idx = present_queue_family_idx.value();
                m_compute_queue_family_idx = compute_queue_family_idx.value();
            }
        }

        COUST_PANIC_IF(m_phydev == VK_NULL_HANDLE,
            "Can't find appropriate physcial device for vulkan");

        VkPhysicalDeviceProperties phydev_props{};
        vkGetPhysicalDeviceProperties(m_phydev, &phydev_props);
        VkSampleCountFlags msaa_cnt_flags =
            phydev_props.limits.framebufferColorSampleCounts &
            phydev_props.limits.framebufferDepthSampleCounts;
        if (msaa_cnt_flags & VK_SAMPLE_COUNT_64_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_64_BIT;
        } else if (msaa_cnt_flags & VK_SAMPLE_COUNT_32_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_32_BIT;
        } else if (msaa_cnt_flags & VK_SAMPLE_COUNT_16_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_16_BIT;
        } else if (msaa_cnt_flags & VK_SAMPLE_COUNT_8_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_8_BIT;
        } else if (msaa_cnt_flags & VK_SAMPLE_COUNT_4_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_4_BIT;
        } else if (msaa_cnt_flags & VK_SAMPLE_COUNT_2_BIT) {
            m_max_msaa_sample = VK_SAMPLE_COUNT_2_BIT;
        }
    }

    {
        memory::robin_set<uint32_t, DefaultAlloc> queue_family_indices{
            {m_graphics_queue_family_idx, m_present_queue_family_idx,
             m_compute_queue_family_idx},
            3, get_default_alloc()
        };
        memory::vector<VkDeviceQueueCreateInfo, DefaultAlloc> queue_infos{
            get_default_alloc()};
        queue_infos.reserve(queue_family_indices.size());
        float const queue_priority = 1.0f;
        for (uint32_t const idx : queue_family_indices) {
            queue_infos.push_back(VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = idx,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            });
        }
        VkDeviceCreateInfo dev_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = device_creation_pnext,
            .flags = 0,
            .queueCreateInfoCount = (uint32_t) queue_infos.size(),
            .pQueueCreateInfos = queue_infos.data(),
            .enabledExtensionCount = (uint32_t) device_extension.size(),
            .ppEnabledExtensionNames = device_extension.data(),
            .pEnabledFeatures = &required_phydev_feature,
        };
        COUST_VK_CHECK(vkCreateDevice(m_phydev, &dev_info,
                           COUST_VULKAN_ALLOC_CALLBACK, &m_dev),
            "Can't create vulkan device");

        volkLoadDevice(m_dev);

        vkGetDeviceQueue(
            m_dev, m_graphics_queue_family_idx, 0, &m_graphics_queue);
        vkGetDeviceQueue(
            m_dev, m_present_queue_family_idx, 0, &m_present_queue);
        vkGetDeviceQueue(
            m_dev, m_compute_queue_family_idx, 0, &m_compute_queue);

        VmaVulkanFunctions vma_loading_func{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };
        VmaAllocatorCreateInfo vma_alloc_info{
            .physicalDevice = m_phydev,
            .device = m_dev,
            .pVulkanFunctions = &vma_loading_func,
            .instance = m_instance,
            .vulkanApiVersion = COUST_VULKAN_API_VERSION,
        };
        COUST_VK_CHECK(vmaCreateAllocator(&vma_alloc_info, &m_vma_alloc),
            "Can't create VMA allocator");
    }

    m_cmdbuf_cache.initialize(
        m_dev, m_graphics_queue, m_graphics_queue_family_idx);

    m_graphics_pipeline_cache.initialize(m_dev, m_phydev);

    m_fbo_cache.initialize(m_dev);

    m_sampler_cache.initialize(m_dev);

    m_stage_pool.initialize(m_dev, m_vma_alloc);

    m_swapchain.initialize(m_dev, m_phydev, m_surface,
        m_graphics_queue_family_idx, m_present_queue_family_idx,
        m_cmdbuf_cache.get());

    m_cmdbuf_cache.get().set_command_buffer_changed_callback(
        [this](VulkanCommandBuffer const& buf) {
            m_graphics_pipeline_cache.get().gc(buf);
        });

    m_swapchain.get().prepare();
    m_swapchain.get().create();
}

VulkanDriver::~VulkanDriver() noexcept {
    m_swapchain.get().destroy();

    m_cmdbuf_cache.destroy();

    m_fbo_cache.get().reset();

    m_graphics_pipeline_cache.get().reset();

    m_stage_pool.get().reset();

    m_sampler_cache.get().reset();

    vmaDestroyAllocator(m_vma_alloc);
    vkDestroyDevice(m_dev, COUST_VULKAN_ALLOC_CALLBACK);
    vkDestroySurfaceKHR(m_instance, m_surface, COUST_VULKAN_ALLOC_CALLBACK);
#if defined(COUST_VK_DBG)
    vkDestroyDebugUtilsMessengerEXT(
        m_instance, m_dbg_messenger, COUST_VULKAN_ALLOC_CALLBACK);
#endif
    vkDestroyInstance(m_instance, COUST_VULKAN_ALLOC_CALLBACK);
}

}  // namespace render
}  // namespace coust
