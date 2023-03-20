#include "pch.h"

#include "Coust/Render/Vulkan/VulkanDriver.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/VulkanPipeline.h"

#include "Coust/Core/Window.h"
#include "Coust/Utils/FileSystem.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>

namespace Coust::Render::VK
{

    inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        /* Redundant validation error message */

        // if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        // 	COUST_CORE_INFO(pCallbackData->pMessage);
        // else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        // 	COUST_CORE_WARN(pCallbackData->pMessage);
        // else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        // 	COUST_CORE_ERROR(pCallbackData->pMessage);

        /* Redundant validation error message */

        return VK_FALSE;
    }

    inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback
    (
        VkDebugReportFlagsEXT      flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t                   object,
        size_t                     location,
        int32_t                    messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* UserData
    )
    {
        // https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
        // This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
        // if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            // return VK_FALSE;

        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            COUST_CORE_TRACE("{0}: {1}", pLayerPrefix, pMessage);
        else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
            COUST_CORE_INFO("{0}: {1}", pLayerPrefix, pMessage);
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            COUST_CORE_WARN("{0}: {1}", pLayerPrefix, pMessage);
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            COUST_CORE_WARN("{0}: {1}", pLayerPrefix, pMessage);
        else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            COUST_CORE_ERROR("{0}: {1}", pLayerPrefix, pMessage);

        return VK_FALSE;
    }

    inline VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo()
    {
        return VkDebugUtilsMessengerCreateInfoEXT
        {
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity    =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType        =	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback    = DebugCallback,
        };
    }

    inline VkDebugReportCallbackCreateInfoEXT DebugReportCallbackCreateInfo()
    {
        return VkDebugReportCallbackCreateInfoEXT
        {
            .sType =			VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            .flags =			VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
            .pfnCallback =		DebugReportCallback,
        };
    }

    Driver::Driver()
        : m_StagePool(m_Context),
          m_Swapchain(m_Context)
    {
        VK_REPORT(volkInitialize(), m_IsInitialized);

        m_IsInitialized = m_IsInitialized && CreateInstance() &&
#ifndef COUST_FULL_RELEASE
            CreateDebugMessengerAndReportCallback() &&
#endif
            CreateSurface() &&
            SelectPhysicalDeviceAndCreateDevice();

        m_Context.CmdBufCacheGraphics = new CommandBufferCache{ m_Context, false };
        m_Swapchain.Prepare();
        m_IsInitialized = m_Swapchain.Create() && m_IsInitialized;
    }

    Driver::~Driver()
    {
        m_IsInitialized = false;

        m_FBOCache.Reset();
        m_Swapchain.Destroy();
        delete m_Context.CmdBufCacheGraphics;
        m_StagePool.Reset();

        vmaDestroyAllocator(m_Context.VmaAlloc);
        vkDestroyDevice(m_Context.Device, nullptr);
        vkDestroySurfaceKHR(m_Context.Instance, m_Context.Surface, nullptr);
#ifndef COUST_FULL_RELEASE
        vkDestroyDebugReportCallbackEXT(m_Context.Instance, m_Context.DebugReportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(m_Context.Instance, m_Context.DebugMessenger, nullptr); 
#endif
        vkDestroyInstance(m_Context.Instance, nullptr);
    }

    bool Driver::CreateInstance()
    {
        VkApplicationInfo appInfo
        {
              .pApplicationName    = "Coust",
            .applicationVersion  = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName         = "No Engine",
            .engineVersion       = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion          = VULKAN_API_VERSION,
        };

        VkInstanceCreateInfo instanceInfo
        {
            .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo
        };

          std::vector<const char*> requiredExtensions
        { 
#ifndef COUST_FULL_RELEASE
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        };
        {
            uint32_t glfwRequiredExtensionCount = 0;
            const char** glfwRequiredExtensionNames = nullptr;
            glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
            requiredExtensions.reserve(1 + glfwRequiredExtensionCount);
            for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
            {
                requiredExtensions.push_back(glfwRequiredExtensionNames[i]);
            }
            uint32_t providedExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &providedExtensionCount, nullptr);
            std::vector<VkExtensionProperties> providedExtensions(providedExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &providedExtensionCount, providedExtensions.data());
            for (const char* requiredEXT : requiredExtensions)
            {
                bool found = false;
                for (const auto& providedEXT : providedExtensions)
                {
                    if (strcmp(requiredEXT, providedEXT.extensionName) == 0)
                        found = true;
                }
                if (!found)
                {
                    COUST_CORE_ERROR("Required extension not found when creating vulkan instance");
                    return false;
                }
            }
            instanceInfo.enabledExtensionCount   = (uint32_t) requiredExtensions.size();
            instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();
        }
 
        std::vector<const char*> requiredLayers = 
        {
#ifndef COUST_FULL_RELEASE
            "VK_LAYER_KHRONOS_validation"
#endif
        };
        {
            uint32_t providedLayerCount = 0;
            vkEnumerateInstanceLayerProperties(&providedLayerCount, nullptr);
            std::vector<VkLayerProperties> providedLayers(providedLayerCount);
            vkEnumerateInstanceLayerProperties(&providedLayerCount, providedLayers.data());
            for (const char* requiredLayer : requiredLayers)
            {
                bool found = false;
                for (const auto& providedLayer : providedLayers)
                {
                    if (strcmp(requiredLayer, providedLayer.layerName) == 0)
                        found = true;
                }
                if (!found)
                {
                    COUST_CORE_ERROR("Required layer not found when creating vulkan instance");
                    return false;
                }
            }
            instanceInfo.enabledLayerCount = (uint32_t) requiredLayers.size();
            instanceInfo.ppEnabledLayerNames = requiredLayers.data();
        }
 
#ifndef COUST_FULL_RELEASE
           VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = DebugMessengerCreateInfo();
         instanceInfo.pNext                       = (void*)&debugMessengerCreateInfo;
#endif

        VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Context.Instance));

        volkLoadInstance(m_Context.Instance);
        return true;
    }

    bool Driver::CreateDebugMessengerAndReportCallback()
    {
        {
            VkDebugUtilsMessengerCreateInfoEXT info = DebugMessengerCreateInfo();
               VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_Context.Instance, &info, nullptr, &m_Context.DebugMessenger));
        }

        {
            VkDebugReportCallbackCreateInfoEXT info = DebugReportCallbackCreateInfo();
            VK_CHECK(vkCreateDebugReportCallbackEXT(m_Context.Instance, &info, nullptr, &m_Context.DebugReportCallback));
        }

        return true;
    }

    bool Driver::CreateSurface()
    {
        VK_CHECK(glfwCreateWindowSurface(m_Context.Instance, GlobalContext::Get().GetWindow().GetHandle(), nullptr, &m_Context.Surface));
        return true;
    }

    bool Driver::SelectPhysicalDeviceAndCreateDevice()
    {
        std::vector <const char*> requiredDeviceExtensions
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        };
        {
            uint32_t physicalDeviceCount = 0;
            vkEnumeratePhysicalDevices(m_Context.Instance, &physicalDeviceCount, nullptr);
            if (physicalDeviceCount == 0)
            {
                COUST_CORE_ERROR("Not physical device with vulkan support found");
                return false;
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(m_Context.Instance, &physicalDeviceCount, physicalDevices.data());

            int highestScore = -1;
            for (const auto& device : physicalDevices)
            {
                std::optional<uint32_t> graphicsQueueFamilyIndex, presentQueueFamilyIndex;
                {
                    uint32_t physicalDeviceQueueFamilyCount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, nullptr);
                    std::vector<VkQueueFamilyProperties> queueFamilyProperties(physicalDeviceQueueFamilyCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, queueFamilyProperties.data());
                    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
                    {
                        VkBool32 supportPresent = VK_FALSE;
                        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Context.Surface, &supportPresent);
                        VkBool32 supportGraphics = queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                        // Try finding a queue that can support present & rendering at the same time
                        if (supportPresent && supportGraphics)
                        {
                            presentQueueFamilyIndex = i;
                            graphicsQueueFamilyIndex = i;
                            break;
                        }
                        if (!presentQueueFamilyIndex.has_value() && supportPresent)
                            presentQueueFamilyIndex = i;
                        if (!graphicsQueueFamilyIndex.has_value() && supportGraphics)
                            graphicsQueueFamilyIndex = i;
                    }

                    // Try finding another queue that support compute
                    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
                    {
                        const bool supportCompute = (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
                        if (supportCompute)
                        {
                            if (m_Context.ComputeQueueFamilyIndex == INVALID_IDX)
                                m_Context.ComputeQueueFamilyIndex = i;
                            if (graphicsQueueFamilyIndex.has_value() && graphicsQueueFamilyIndex.value() != i)
                            {
                                m_Context.ComputeQueueFamilyIndex = i;
                                break;
                            }
                        }
                        

                    }
                }

                bool requiredExtensionSupported = false;
                {
                    uint32_t deviceExtensionCount = 0;
                    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
                    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
                    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExtensions.data());
                    std::unordered_set<std::string> requiredExtensions{ requiredDeviceExtensions.begin(), requiredDeviceExtensions.end() };
                    for (const auto& extension : deviceExtensions)
                    {
                        requiredExtensions.erase(std::string{ extension.extensionName });
                    }
                    requiredExtensionSupported = requiredExtensions.empty();
                }

                bool requiredFeatureSupported = false;
                {
                    VkPhysicalDeviceFeatures features{};
                    vkGetPhysicalDeviceFeatures(device, &features);
                    requiredFeatureSupported = features.samplerAnisotropy != VK_FALSE;
                }

                VkPhysicalDeviceProperties physicalDeviceProperties;
                vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
                // Get MSAA sample count
                {
                    VkSampleCountFlags MSAACountFlags = physicalDeviceProperties.limits.framebufferColorSampleCounts & 
                        physicalDeviceProperties.limits.framebufferDepthSampleCounts;
                    if (MSAACountFlags & VK_SAMPLE_COUNT_64_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_64_BIT;
                    else if (MSAACountFlags & VK_SAMPLE_COUNT_32_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_32_BIT;
                    else if (MSAACountFlags & VK_SAMPLE_COUNT_16_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_16_BIT;
                    else if (MSAACountFlags & VK_SAMPLE_COUNT_8_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_8_BIT;
                    else if (MSAACountFlags & VK_SAMPLE_COUNT_4_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_4_BIT;
                    else if (MSAACountFlags & VK_SAMPLE_COUNT_2_BIT)
                        m_Context.MSAASampleCount = VK_SAMPLE_COUNT_2_BIT;
                }

                if (requiredExtensionSupported && requiredFeatureSupported && 
                    presentQueueFamilyIndex.has_value() && graphicsQueueFamilyIndex.has_value())
                {
                    int score = 0;
                    if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                        score += 1000;
                    else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                        score += 100;
                    if (presentQueueFamilyIndex.value() == graphicsQueueFamilyIndex.value())
                        score += 50;
                    if (score > highestScore)
                    {
                        highestScore = score;
                        m_Context.PhysicalDevice = device;
                        m_Context.PresentQueueFamilyIndex = presentQueueFamilyIndex.value();
                        m_Context.GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex.value();
                    }
                }
            }
            if (m_Context.PhysicalDevice == VK_NULL_HANDLE)
            {
                COUST_CORE_ERROR("No suitable physical device found");
                return false;
            }

            vkGetPhysicalDeviceProperties(m_Context.PhysicalDevice, m_Context.GPUProperties.get());
        }
 
        {
            std::unordered_set<uint32_t> queueFamilyIndices =
            {
                m_Context.PresentQueueFamilyIndex,
                m_Context.GraphicsQueueFamilyIndex,
                m_Context.ComputeQueueFamilyIndex,
            };
            std::vector<VkDeviceQueueCreateInfo> deviceQueueInfo{};
            float queuePriority = 1.0f;
            for (const auto& familyIndex : queueFamilyIndices)
            {
                VkDeviceQueueCreateInfo info
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = familyIndex,
                    .queueCount = 1,
                    .pQueuePriorities = &queuePriority
                };
                deviceQueueInfo.push_back(info);
            }

            {
                // Enable features
                VkPhysicalDeviceFeatures physicalDeviceFeatures
                {
                    .samplerAnisotropy = VK_TRUE
                };
                VkPhysicalDeviceShaderDrawParametersFeatures shaderFeatures
                {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
                    .shaderDrawParameters = VK_TRUE,
                };
                VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures
                {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                    .pNext = &shaderFeatures,
                    .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                    .descriptorBindingVariableDescriptorCount = VK_TRUE,
                    .runtimeDescriptorArray = VK_TRUE,
                };

                VkPhysicalDeviceSynchronization2Features physicalDeviceSynchronization2Features
                {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                    .pNext = &physicalDeviceDescriptorIndexingFeatures,
                    .synchronization2 = VK_TRUE,
                };

                VkDeviceCreateInfo deviceInfo
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                    .pNext = &physicalDeviceSynchronization2Features,
                    .queueCreateInfoCount = (uint32_t)deviceQueueInfo.size(),
                    .pQueueCreateInfos = deviceQueueInfo.data(),
                    .enabledLayerCount = 0,
                    .enabledExtensionCount = (uint32_t)requiredDeviceExtensions.size(),
                    .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
                    .pEnabledFeatures = &physicalDeviceFeatures,
                };
                VK_CHECK(vkCreateDevice(m_Context.PhysicalDevice, &deviceInfo, nullptr, &m_Context.Device));
            }

            volkLoadDevice(m_Context.Device);

            vkGetDeviceQueue(m_Context.Device, m_Context.PresentQueueFamilyIndex, 0, &m_Context.PresentQueue);
            vkGetDeviceQueue(m_Context.Device, m_Context.GraphicsQueueFamilyIndex, 0, &m_Context.GraphicsQueue);
            vkGetDeviceQueue(m_Context.Device, m_Context.ComputeQueueFamilyIndex, 0, &m_Context.ComputeQueue);

            {
                VmaVulkanFunctions functions
                {
                    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                    .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                };
                VmaAllocatorCreateInfo vmaAllocInfo 
                {
                    .physicalDevice		= m_Context.PhysicalDevice,
                    .device				= m_Context.Device,
                    .pVulkanFunctions	= &functions,
                    .instance			= m_Context.Instance,
                    .vulkanApiVersion	= VULKAN_API_VERSION,
                };
                VK_CHECK(vmaCreateAllocator(&vmaAllocInfo, &m_Context.VmaAlloc));
            }
        }
        return true;
    }

    void Driver::InitializationTest()
    {
        RenderPass::ConstructParam rpp 
        {
			.ctx = m_Context,
			.colorFormat = { m_Swapchain.SurfaceFormat.format },
			.depthFormat = m_Swapchain.DepthFormat,
			.clearMask = 0u,
			.discardStartMask = COLOR0 | DEPTH,
			.discardEndMask = COLOR0 | DEPTH,
			.sample = VK_SAMPLE_COUNT_1_BIT,
			.resolveMask = 0u,
			.inputAttachmentMask = 0u,
            .depthResolve = false,
			.dedicatedName = "Test render pass",
        };
        const RenderPass* rp = m_FBOCache.GetRenderPass(rpp);
        if (!rp)
            return;
        
        ShaderSource vs{ FileSystem::GetFullPathFrom({ "Coust", "shaders", "triangle.vert"}) };
        ShaderSource fs{ FileSystem::GetFullPathFrom({ "Coust", "shaders", "triangle.frag"}) };

        ShaderModule::ConstructParm vssmp 
        {
            .ctx = m_Context,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .source = vs,
            .dedicatedName = "Test vertex shader module",
        };
        ShaderModule vssm{ vssmp };
        if (!ShaderModule::CheckValidation(vssm))
            return;

        ShaderModule::ConstructParm fssmp
        {
            .ctx = m_Context,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .source = fs,
            .dedicatedName = "Test fragment shader module",
        };
        ShaderModule fssm { fssmp };
        if (!ShaderModule::CheckValidation(fssm))
            return;
        
        PipelineLayout::ConstructParam plp 
        {
            .ctx = m_Context,
            .dedicatedName = "Test pipeline layout",
        };
        plp.shaderModules.push_back(&vssm);
        plp.shaderModules.push_back(&fssm);
        PipelineLayout pl { plp };
        if (!PipelineLayout::CheckValidation(pl))
            return;
        
        GraphicsPipeline::ConstructParam gpp 
        {
            .ctx = m_Context,
            .specializationConstantInfo = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .colorTargetCount = 1,
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .layout = pl,
            .renderPass = *rp,
            .subpassIdx = 0,
            .cache = VK_NULL_HANDLE,
            .dedicatedName = "Test graphics pipeline",
        };
        GraphicsPipeline gp { gpp };
        if (!GraphicsPipeline::CheckValidation(gp))
            return;
        
        m_Context.CmdBufCacheGraphics->Flush();
        m_Context.CmdBufCacheGraphics->Wait();
    }

    void Driver::LoopTest()
    {
        m_Context.CmdBufCacheGraphics->GC();
        m_StagePool.GC();
        m_FBOCache.GC();
    }
}