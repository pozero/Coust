#include "core/Application.h"
#include "pch.h"
#include "render/vulkan/VulkanDriver.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanVersion.h"
#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "vk_mem_alloc.h"
#include "volk.h"

WARNING_POP

namespace coust {
namespace render {
namespace detail {

constexpr memory::string<DefaultAlloc> get_input_attachment_name(
    uint32_t idx) noexcept {
    COUST_ASSERT(idx < MAX_ATTACHMENT_COUNT, "");
    switch (idx) {
        case 0:
            return "INPUT_ATTACHMENT_ZERO";
        case 1:
            return "INPUT_ATTACHMENT_ONE";
        case 2:
            return "INPUT_ATTACHMENT_TWO";
        case 3:
            return "INPUT_ATTACHMENT_THREE";
        case 4:
            return "INPUT_ATTACHMENT_FOUR";
        case 5:
            return "INPUT_ATTACHMENT_FIVE";
        case 6:
            return "INPUT_ATTACHMENT_SIX";
        case 7:
            return "INPUT_ATTACHMENT_SEVEN";
    }
    ASSUME(0);
}

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

    m_graphics_cmdbuf_cache.initialize(
        m_dev, m_graphics_queue, m_graphics_queue_family_idx);

    m_compute_cmdbuf_cache.initialize(
        m_dev, m_compute_queue, m_compute_queue_family_idx);

    m_shader_pool.initialize(m_dev);

    m_descriptor_cache.initialize(m_dev, m_phydev);

    m_graphics_pipeline_cache.initialize(
        m_dev, m_shader_pool.get(), m_descriptor_cache.get());

    m_compute_pipeline_cache.initialize(
        m_dev, m_shader_pool.get(), m_descriptor_cache.get());

    m_fbo_cache.initialize(m_dev);

    m_sampler_cache.initialize(m_dev);

    m_stage_pool.initialize(m_dev, m_vma_alloc);

    m_swapchain.initialize(m_dev, m_phydev, m_surface,
        m_graphics_queue_family_idx, m_present_queue_family_idx,
        m_graphics_cmdbuf_cache.get());

    m_graphics_cmdbuf_cache.get().set_command_buffer_changed_callback(
        [this](VulkanCommandBuffer const& buf) {
            m_graphics_pipeline_cache.get().gc(buf);
        });

    m_compute_cmdbuf_cache.get().set_command_buffer_changed_callback(
        [this](VulkanCommandBuffer const& buf) {
            m_compute_pipeline_cache.get().gc(buf);
        });

    m_swapchain.get().prepare();
    m_swapchain.get().create();
}

VulkanDriver::~VulkanDriver() noexcept {
    m_graphics_cmdbuf_cache.destroy();

    m_compute_cmdbuf_cache.destroy();

    m_swapchain.get().destroy();

    m_fbo_cache.get().reset();

    m_shader_pool.get().reset();

    m_descriptor_cache.get().reset();

    m_graphics_pipeline_cache.get().reset();

    m_compute_pipeline_cache.get().reset();

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

void VulkanDriver::wait() noexcept {
    m_graphics_cmdbuf_cache.get().wait();
    m_compute_cmdbuf_cache.get().wait();
}

void VulkanDriver::gc() noexcept {
    m_graphics_cmdbuf_cache.get().gc();
    m_compute_cmdbuf_cache.get().gc();
    m_descriptor_cache.get().gc();
    m_stage_pool.get().gc();
    m_fbo_cache.get().gc();
}

void VulkanDriver::begin_frame() noexcept {
}

void VulkanDriver::end_frame() noexcept {
    if (m_graphics_cmdbuf_cache.get().flush() &&
        m_compute_cmdbuf_cache.get().flush()) {
        gc();
    }
}

VkSampler VulkanDriver::create_sampler(
    VulkanSamplerParam const& param) noexcept {
    return m_sampler_cache.get().get(param);
}

VulkanBuffer VulkanDriver::create_buffer_single_queue(VkDeviceSize size,
    VkBufferUsageFlags vk_buf_usage, VulkanBuffer::Usage usage) noexcept {
    std::array<uint32_t, 3> constexpr related_queues{
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
    };
    return VulkanBuffer{
        m_dev, m_vma_alloc, size, vk_buf_usage, usage, related_queues};
}

VulkanVertexIndexBuffer VulkanDriver::create_vertex_index_buffer(
    MeshAggregate const& mesh_aggregate) noexcept {
    return VulkanVertexIndexBuffer{m_dev, m_vma_alloc,
        m_graphics_cmdbuf_cache.get().get(), m_stage_pool.get(),
        mesh_aggregate};
}

VulkanTransformationBuffer VulkanDriver::create_transformation_buffer(
    MeshAggregate const& mesh_aggregate) noexcept {
    return VulkanTransformationBuffer{m_dev, m_graphics_queue_family_idx,
        m_compute_queue_family_idx, m_vma_alloc,
        m_graphics_cmdbuf_cache.get().get(), m_stage_pool.get(),
        mesh_aggregate};
}

VulkanMaterialBuffer VulkanDriver::create_material_buffer(
    MeshAggregate const& mesh_aggregate) noexcept {
    return VulkanMaterialBuffer{m_dev, m_vma_alloc,
        m_graphics_cmdbuf_cache.get().get(), m_stage_pool.get(),
        mesh_aggregate};
}

VulkanImage VulkanDriver::create_image_single_queue(uint32_t width,
    uint32_t height, uint32_t levels, VkSampleCountFlagBits samples,
    VkFormat format, VulkanImage::Usage usage) noexcept {
    std::array<uint32_t, 3> constexpr related_queues{
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
    };
    return VulkanImage{m_dev, m_vma_alloc, m_graphics_cmdbuf_cache.get().get(),
        width, height, format, usage, 0, 0, related_queues, levels, samples};
}

VulkanRenderTarget VulkanDriver::create_render_target(uint32_t width,
    uint32_t height, VkSampleCountFlagBits samples,
    std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> const& colors,
    VulkanAttachment const& depth) noexcept {
    return VulkanRenderTarget{
        m_dev, m_vma_alloc, m_graphics_cmdbuf_cache.get().get(),
        {width, height},
          samples, colors, depth
    };
}

VulkanRenderTarget const& VulkanDriver::get_attached_render_target()
    const noexcept {
    return m_attached_render_target;
}

void VulkanDriver::update_buffer(VulkanBuffer& buffer,
    std::span<const uint8_t> data, size_t offset) noexcept {
    buffer.update(
        m_stage_pool.get(), m_graphics_cmdbuf_cache.get().get(), data, offset);
}

void VulkanDriver::update_image(VulkanImage& image, VkFormat format,
    uint32_t width, uint32_t height, const void* data, uint32_t dst_level,
    uint32_t dst_layer, uint32_t dst_layer_cnt) noexcept {
    image.update(m_stage_pool.get(), m_graphics_cmdbuf_cache.get().get(),
        format, width, height, data, dst_layer, dst_layer_cnt, dst_level);
}

void VulkanDriver::begin_render_pass(VulkanRenderTarget const& render_target,
    AttachmentFlags clear_mask, AttachmentFlags discard_start_mask,
    AttachmentFlags discard_end_mask, uint8_t input_attachment_mask,
    VkClearValue clear_val) noexcept {
    uint8_t present_src_mask = 0u;
    if (render_target.is_attached_to_swapchain()) {
        discard_start_mask |= AttachmentFlagBits::COLOR0;
        present_src_mask |= AttachmentFlagBits::COLOR0;
    }
    bool const multisampled_rt =
        render_target.get_sample_count() != VK_SAMPLE_COUNT_1_BIT;
    VulkanRenderPass::Param rp_param{
        .color_formats =
            {
                            render_target.get_color_format(0),
                            render_target.get_color_format(1),
                            render_target.get_color_format(2),
                            render_target.get_color_format(3),
                            render_target.get_color_format(4),
                            render_target.get_color_format(5),
                            render_target.get_color_format(6),
                            render_target.get_color_format(7),
                            },
        .depth_format = render_target.get_depth_format(),
        .clear_mask = clear_mask,
        .discard_start_mask = discard_start_mask,
        .discard_end_mask = discard_end_mask,
        .sample = render_target.get_sample_count(),
        .input_attachment_mask = input_attachment_mask,
        .present_src_mask = present_src_mask,
        .depth_resolve = multisampled_rt,
    };
    if (multisampled_rt) {
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
            VulkanAttachment const& attach = render_target.get_color(i);
            if (attach.m_image &&
                attach.m_image->get_sample_count() == VK_SAMPLE_COUNT_1_BIT) {
                rp_param.resolve_mask |= 1 << i;
            }
        }
    }
    VulkanRenderPass const& render_pass =
        m_fbo_cache.get().get_render_pass(rp_param);
    m_graphics_pipeline_cache.get().bind_render_pass(render_pass, 0);
    VulkanFramebuffer::Param fb_param{
        .width = render_target.get_extent().width,
        .height = render_target.get_extent().height,
        .layers = 1,
        .render_pass = &render_pass,
    };
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        auto const& attachment = render_target.get_color(i);
        auto const& msaa_attachment = render_target.get_color_msaa(i);
        if (!attachment.m_image) {
            fb_param.colors[i] = nullptr;
            fb_param.resolves[i] = nullptr;
        } else if (!multisampled_rt) {
            fb_param.colors[i] =
                attachment.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT);
            fb_param.resolves[i] = nullptr;
        } else {
            fb_param.colors[i] =
                msaa_attachment.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT);
            if (attachment.m_image->get_sample_count() ==
                VK_SAMPLE_COUNT_1_BIT) {
                fb_param.resolves[i] =
                    attachment.get_image_view(VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }
    }
    if (render_target.get_depth().m_image) {
        if (multisampled_rt) {
            fb_param.depth = render_target.get_depth_msaa().get_image_view(
                VK_IMAGE_ASPECT_DEPTH_BIT);
            if (render_target.get_depth().m_image->get_sample_count() ==
                VK_SAMPLE_COUNT_1_BIT) {
                fb_param.depth_resolve =
                    render_target.get_depth().get_image_view(
                        VK_IMAGE_ASPECT_DEPTH_BIT);
            }
        } else {
            fb_param.depth = render_target.get_depth().get_image_view(
                VK_IMAGE_ASPECT_DEPTH_BIT);
            fb_param.depth_resolve = nullptr;
        }
    }
    VulkanFramebuffer const& framebuffer =
        m_fbo_cache.get().get_framebuffer(fb_param);
    std::array<VkClearValue,
        MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1 + 1>
        clear_values{};
    std::ranges::fill(clear_values, clear_val);
    VkRenderPassBeginInfo rp_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass.get_handle(),
        .framebuffer = framebuffer.get_handle(),
        .renderArea =
            {
                         .offset = {},
                         .extent = render_target.get_extent(),
                         },
        .clearValueCount = render_target.get_attachment_cnt(),
        .pClearValues = clear_values.data(),
    };
    VkCommandBuffer cmdbuf = m_graphics_cmdbuf_cache.get().get();
    vkCmdBeginRenderPass(cmdbuf, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) m_swapchain.get().m_extent.width,
        .height = (float) m_swapchain.get().m_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    m_cur_render_target = &render_target;
    m_cur_render_pass = &render_pass;
    m_cur_subpass = 0;
    m_cur_subpass_mask = rp_param.input_attachment_mask;
}

void VulkanDriver::next_subpass() noexcept {
    ++m_cur_subpass;
    vkCmdNextSubpass(
        m_graphics_cmdbuf_cache.get().get(), VK_SUBPASS_CONTENTS_INLINE);
    m_graphics_pipeline_cache.get().bind_render_pass(
        *m_cur_render_pass, m_cur_subpass);
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        if ((m_cur_subpass_mask & (1 << i)) != 0) {
            VulkanAttachment const& input_attachment =
                m_cur_render_target->get_color(i);
            m_graphics_pipeline_cache.get().bind_input_attachment(
                detail::get_input_attachment_name(i), input_attachment);
        }
    }
}

void VulkanDriver::end_render_pass() noexcept {
    VkCommandBuffer cmdbuf = m_graphics_cmdbuf_cache.get().get();
    vkCmdEndRenderPass(cmdbuf);
    m_cur_render_target = nullptr;
    m_cur_render_pass = nullptr;
    m_cur_subpass = 0;
    m_cur_subpass_mask = 0;
}

void VulkanDriver::update_swapchain() noexcept {
    if (m_swapchain.get().m_is_next_img_acquired) {
        m_attached_render_target.attach(m_swapchain.get());
        return;
    }
    if (m_swapchain.get().has_resized()) {
        refresh_swapchain();
    }
    m_swapchain.get().acquire();
    m_attached_render_target.attach(m_swapchain.get());
}

void VulkanDriver::graphics_commit_present() noexcept {
    if (m_graphics_cmdbuf_cache.get().flush()) {
        gc();
    }
    m_swapchain.get().m_is_next_img_acquired = false;
    VkSemaphore const last_submission_signal =
        m_graphics_cmdbuf_cache.get().get_last_submission_singal();
    VkSwapchainKHR const sc = m_swapchain.get().get_handle();
    uint32_t const cur_img_idx = m_swapchain.get().get_image_idx();
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &last_submission_signal,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &cur_img_idx,
    };
    COUST_VK_CHECK(vkQueuePresentKHR(m_present_queue, &present_info), "");
}

void VulkanDriver::commit_compute() noexcept {
    m_compute_cmdbuf_cache.get().flush();
}

SpecializationConstantInfo& VulkanDriver::bind_specialization_info(
    VkPipelineBindPoint bind_point) noexcept {
    COUST_ASSERT(bind_point == VK_PIPELINE_BIND_POINT_COMPUTE ||
                     bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS,
        "");
    if (bind_point == VK_PIPELINE_BIND_POINT_COMPUTE) {
        return m_compute_pipeline_cache.get().bind_specialization_constant();
    } else {
        return m_graphics_pipeline_cache.get().bind_specialization_constant();
    }
}

void VulkanDriver::bind_shader(VkPipelineBindPoint bind_point,
    VkShaderStageFlagBits vk_shader_stage,
    std::filesystem::path source_path) noexcept {
    COUST_ASSERT(bind_point == VK_PIPELINE_BIND_POINT_COMPUTE ||
                     bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS,
        "");
    ShaderSource source{source_path};
    if (bind_point == VK_PIPELINE_BIND_POINT_COMPUTE) {
        m_compute_pipeline_cache.get().bind_shader({vk_shader_stage, source});
    } else {
        m_graphics_pipeline_cache.get().bind_shader({vk_shader_stage, source});
    }
}

void VulkanDriver::set_update_mode(VkPipelineBindPoint bind_point,
    std::string_view name, ShaderResourceUpdateMode update_mode) noexcept {
    if (bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS &&
        update_mode == ShaderResourceUpdateMode::update_after_bind) {
        for (auto& module :
            m_graphics_pipeline_cache.get().get_cur_shader_modules()) {
            module->set_update_after_bind_image(name);
        }
    }
}

void VulkanDriver::bind_buffer_whole(VkPipelineBindPoint bind_point,
    std::string_view name, VulkanBuffer const& buffer,
    uint32_t array_idx) noexcept {
    COUST_ASSERT(bind_point == VK_PIPELINE_BIND_POINT_COMPUTE ||
                     bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS,
        "");
    if (bind_point == VK_PIPELINE_BIND_POINT_COMPUTE) {
        m_compute_pipeline_cache.get().bind_buffer(
            name, buffer, 0, VK_WHOLE_SIZE, array_idx);
    } else {
        m_graphics_pipeline_cache.get().bind_buffer(
            name, buffer, 0, VK_WHOLE_SIZE, array_idx, false);
    }
}

void VulkanDriver::bind_buffer(VkPipelineBindPoint bind_point,
    std::string_view name, VulkanBuffer const& buffer, uint64_t offset,
    uint64_t size, uint32_t array_idx) noexcept {
    COUST_ASSERT(bind_point == VK_PIPELINE_BIND_POINT_COMPUTE ||
                     bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS,
        "");
    if (bind_point == VK_PIPELINE_BIND_POINT_COMPUTE) {
        m_compute_pipeline_cache.get().bind_buffer(
            name, buffer, offset, size, array_idx);
    } else {
        m_graphics_pipeline_cache.get().bind_buffer(
            name, buffer, offset, size, array_idx, false);
    }
}

void VulkanDriver::bind_image(VkPipelineBindPoint bind_point,
    std::string_view name, VkSampler sampler, VulkanImage const& image,
    uint32_t array_idx) noexcept {
    COUST_ASSERT(bind_point == VK_PIPELINE_BIND_POINT_COMPUTE ||
                     bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS,
        "");
    if (bind_point == VK_PIPELINE_BIND_POINT_COMPUTE) {
        m_compute_pipeline_cache.get().bind_image(
            name, sampler, image, array_idx);
    } else {
        m_graphics_pipeline_cache.get().bind_image(
            name, sampler, image, array_idx);
    }
}

void VulkanDriver::calculate_transformation(
    VulkanTransformationBuffer& transformation_buf) noexcept {
    VkCommandBuffer cmdbuf = m_compute_cmdbuf_cache.get().get();
    m_compute_pipeline_cache.get().bind_pipeline_layout();
    VulkanBuffer const& mat_buf = transformation_buf.get_matrices_buffer();
    VulkanBuffer const& mat_idx_buf =
        transformation_buf.get_matrix_indices_buffer();
    VulkanBuffer const& idx_idx_buf =
        transformation_buf.get_index_indices_buffer();
    VulkanBuffer const& dyna_mat_buf =
        transformation_buf.get_dyna_mat_buf(m_stage_pool.get(), cmdbuf);
    VulkanBuffer const& ret_mat_buf =
        transformation_buf.get_result_matrices_buffer();
    m_compute_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::MAT_BUF_NAME, mat_buf, 0,
        mat_buf.get_size(), 0);
    m_compute_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::MAT_IDX_BUF_NAME, mat_idx_buf, 0,
        mat_idx_buf.get_size(), 0);
    m_compute_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::MAT_IDX_IDX_NAME, idx_idx_buf, 0,
        idx_idx_buf.get_size(), 0);
    m_compute_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::DYNA_MAT_NAME, dyna_mat_buf, 0,
        dyna_mat_buf.get_size(), 0);
    m_compute_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::RES_MAT_NAME, ret_mat_buf, 0,
        ret_mat_buf.get_size(), 0);
    m_compute_pipeline_cache.get().bind_descriptor_set(cmdbuf);
    m_compute_pipeline_cache.get().bind_compute_pipeline(cmdbuf);
    auto [x, y, z] = transformation_buf.get_work_group_size();
    vkCmdDispatch(cmdbuf, x, y, z);
}

void VulkanDriver::draw(VulkanVertexIndexBuffer const& vertex_index_buf,
    VulkanTransformationBuffer const& transformation_buf,
    VulkanMaterialBuffer const& material_buf, glm::mat4 const& proj_view_mat,
    VulkanGraphicsPipeline::RasterState const& raster_state,
    VkRect2D scissor) noexcept {
    VkCommandBuffer cmdbuf = m_graphics_cmdbuf_cache.get().get();
    m_graphics_pipeline_cache.get().bind_pipeline_layout();
    m_graphics_pipeline_cache.get().bind_raster_state(raster_state);
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanVertexIndexBuffer::VERTEX_BUF_NAME,
        vertex_index_buf.get_vertex_buf(), 0,
        vertex_index_buf.get_vertex_buf().get_size(), 0, false);
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanVertexIndexBuffer::INDEX_BUF_NAME,
        vertex_index_buf.get_index_buf(), 0,
        vertex_index_buf.get_index_buf().get_size(), 0, false);
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanVertexIndexBuffer::ATTRIB_OFFSET_BUF_NAME,
        vertex_index_buf.get_attrib_offset_buf(), 0,
        vertex_index_buf.get_attrib_offset_buf().get_size(), 0, false);
    VulkanBuffer const& tbuf = transformation_buf.get_result_matrices_buffer();
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanTransformationBuffer::RES_MAT_NAME, tbuf, 0, tbuf.get_size(), 0,
        false);
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanMaterialBuffer::MATERIAL_INDEX_NAME,
        material_buf.get_material_index_buf(), 0,
        material_buf.get_material_index_buf().get_size(), 0, false);
    m_graphics_pipeline_cache.get().bind_buffer(
        VulkanMaterialBuffer::MATERIAL_NAME, material_buf.get_material_buf(), 0,
        material_buf.get_material_buf().get_size(), 0, false);

    m_graphics_pipeline_cache.get().bind_descriptor_set(cmdbuf);
    if (scissor.extent.width != 0 && scissor.extent.height != 0) {
        vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
    } else {
        VkRect2D const whole_screen{
            .offset = {0, 0},
            .extent = m_swapchain.get().m_extent,
        };
        vkCmdSetScissor(cmdbuf, 0, 1, &whole_screen);
    }
    m_graphics_pipeline_cache.get().bind_graphics_pipeline(cmdbuf);
    struct PushConstant {
        glm::mat4 proj_view_mat;
        uint32_t node_idx;
    };
    for (auto const [draw_cmd_byte_offset, draw_count, node_idx] :
        vertex_index_buf.get_node_infos()) {
        PushConstant const push_constant{proj_view_mat, node_idx};
        m_graphics_pipeline_cache.get().push_constant(cmdbuf,
            VK_SHADER_STAGE_VERTEX_BIT,
            {(const uint8_t*) &push_constant, sizeof(push_constant)});
        vkCmdDrawIndirect(cmdbuf,
            vertex_index_buf.get_draw_cmd_buf().get_handle(),
            draw_cmd_byte_offset, draw_count, sizeof(VkDrawIndirectCommand));
    }
}

void VulkanDriver::refresh_swapchain() noexcept {
    m_swapchain.get().destroy();
    m_swapchain.get().create();
    m_fbo_cache.get().reset();
}

void VulkanDriver::add_compute_to_graphics_dependency() noexcept {
    VkSemaphore compute_finished_signal =
        m_compute_cmdbuf_cache.get().get_last_submission_singal();
    m_graphics_cmdbuf_cache.get().inject_dependency(compute_finished_signal);
}

}  // namespace render
}  // namespace coust
