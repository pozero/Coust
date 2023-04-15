#include "pch.h"

#include "Coust/Render/Vulkan/VulkanDriver.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Vulkan/RenderTarget.h"

#include "Coust/Utils/FileSystem.h"

#include "Coust/Core/Window.h"
#include "Coust/Core/GlobalContext.h"

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

    Driver::Driver() noexcept
        : m_StagePool(m_Context),
          m_Swapchain(m_Context),
          m_GraphicsPipeCache(m_Context),
          m_SamplerCache(m_Context),
          m_StackArena("Vulkan Driver Stack", STACK_ARENA_SIZE, Memory::StackAllocator::ALIGNMENT)
    {
        VK_CHECK(volkInitialize(), "Can't initialize volk");

        CreateInstance();
#ifndef COUST_FULL_RELEASE
        CreateDebugMessengerAndReportCallback();
#endif
        CreateSurface();
        SelectPhysicalDeviceAndCreateDevice();

        m_Context.StkArena = &m_StackArena;
        m_Context.CmdBufCacheGraphics = new CommandBufferCache{ m_Context, false };
        m_Context.PresentRenderTarget = new RenderTarget();
        m_Swapchain.Prepare();
        m_IsInitialized = m_Swapchain.Create() && m_IsInitialized;

        m_Context.CmdBufCacheGraphics->SetCommandBufferChangedCallback(
            [this](const CommandBuffer& buf)
            { 
                m_GraphicsPipeCache.GC(buf); 
            });
    }

    Driver::~Driver() noexcept
    {
        m_IsInitialized = false;

        ShutdownTest();

        delete m_Context.PresentRenderTarget;

        m_Swapchain.Destroy();
        delete m_Context.CmdBufCacheGraphics;

        m_Refrigerator.Reset();
        m_FBOCache.Reset();
        m_GraphicsPipeCache.Reset();
        m_StagePool.Reset();
        m_SamplerCache.Reset();

        vmaDestroyAllocator(m_Context.VmaAlloc);
        vkDestroyDevice(m_Context.Device, nullptr);
        vkDestroySurfaceKHR(m_Context.Instance, m_Context.Surface, nullptr);
#ifndef COUST_FULL_RELEASE
        vkDestroyDebugReportCallbackEXT(m_Context.Instance, m_Context.DebugReportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(m_Context.Instance, m_Context.DebugMessenger, nullptr); 
#endif
        vkDestroyInstance(m_Context.Instance, nullptr);
    }

    void Driver::CreateInstance() noexcept
    {
        DEF_STLALLOC(const char*, m_StackArena, constCharAllocator);
        DEF_STLALLOC(VkExtensionProperties, m_StackArena, extPropAllocator);
        DEF_STLALLOC(VkLayerProperties, m_StackArena, layerPropAllocator);

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

        const char* fixedRequiredExtensions[] = 
        { 
#ifndef COUST_FULL_RELEASE
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        };
        uint32_t glfwRequiredExtensionCount = 0;
        const char** glfwRequiredExtensionNames = nullptr;
        glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
        std::vector<const char*, decltype(constCharAllocator)> requiredExtensions{ constCharAllocator };
        requiredExtensions.reserve(sizeof(fixedRequiredExtensions) / sizeof(fixedRequiredExtensions[0]) + glfwRequiredExtensionCount);
        for (const char* ext : fixedRequiredExtensions)
        {
            requiredExtensions.push_back(ext);
        }
        for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
        {
            requiredExtensions.push_back(glfwRequiredExtensionNames[i]);
        }
        uint32_t providedExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &providedExtensionCount, nullptr);
        std::vector<VkExtensionProperties, decltype(extPropAllocator)> providedExtensions{ extPropAllocator };
        providedExtensions.resize(providedExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &providedExtensionCount, providedExtensions.data());
        for (const char* requiredEXT : requiredExtensions)
        {
            bool found = false;
            for (const auto& providedEXT : providedExtensions)
            {
                if (strcmp(requiredEXT, providedEXT.extensionName) == 0)
                    found = true;
            }
            COUST_CORE_PANIC_IF(!found, "Required extension not found when creating vulkan instance");
        }
        instanceInfo.enabledExtensionCount   = (uint32_t) requiredExtensions.size();
        instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();
 
        std::vector<const char*, decltype(constCharAllocator)> requiredLayers{ constCharAllocator };
        // Please make sure the capacity is big enough that there's no other allocation other than this!
        requiredLayers.reserve(4);
#ifndef COUST_FULL_RELEASE
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
        {
            uint32_t providedLayerCount = 0;
            vkEnumerateInstanceLayerProperties(&providedLayerCount, nullptr);
            std::vector<VkLayerProperties, decltype(layerPropAllocator)> providedLayers{ layerPropAllocator };
            providedLayers.resize(providedLayerCount);
            vkEnumerateInstanceLayerProperties(&providedLayerCount, providedLayers.data());
            for (const char* requiredLayer : requiredLayers)
            {
                bool found = false;
                for (const auto& providedLayer : providedLayers)
                {
                    if (strcmp(requiredLayer, providedLayer.layerName) == 0)
                        found = true;
                }
                COUST_CORE_PANIC_IF(!found, "Required layer not found when creating vulkan instance");
            }
            instanceInfo.enabledLayerCount = (uint32_t) requiredLayers.size();
            instanceInfo.ppEnabledLayerNames = requiredLayers.data();
        }
 
#ifndef COUST_FULL_RELEASE
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = DebugMessengerCreateInfo();
        instanceInfo.pNext = (void*)&debugMessengerCreateInfo;
#endif

        VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Context.Instance), "Can't create Vulkan instance");

        volkLoadInstance(m_Context.Instance);
    }

    void Driver::CreateDebugMessengerAndReportCallback() noexcept
    {
        {
            VkDebugUtilsMessengerCreateInfoEXT info = DebugMessengerCreateInfo();
               VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_Context.Instance, &info, nullptr, &m_Context.DebugMessenger), "Can't create debug messenger");
        }

        {
            VkDebugReportCallbackCreateInfoEXT info = DebugReportCallbackCreateInfo();
            VK_CHECK(vkCreateDebugReportCallbackEXT(m_Context.Instance, &info, nullptr, &m_Context.DebugReportCallback), "Can't create debug report callback");
        }
    }

    void Driver::CreateSurface() noexcept
    {
        VK_CHECK(glfwCreateWindowSurface(m_Context.Instance, GlobalContext::Get().GetWindow().GetHandle(), nullptr, &m_Context.Surface), "Can't create surface");
    }

    void Driver::SelectPhysicalDeviceAndCreateDevice() noexcept
    {
        DEF_STLALLOC(VkPhysicalDevice, m_StackArena,  physDevAlloc);
        DEF_STLALLOC(VkQueueFamilyProperties, m_StackArena, queueFamPropAlloc);
        DEF_STLALLOC(VkExtensionProperties, m_StackArena, extPropAlloc);
        DEF_STLALLOC(VkDeviceQueueCreateInfo, m_StackArena, queueCIAlloc);

        const char* requiredDeviceExtensions[]
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        };
        {
            uint32_t physicalDeviceCount = 0;
            vkEnumeratePhysicalDevices(m_Context.Instance, &physicalDeviceCount, nullptr);
            COUST_CORE_PANIC_IF(physicalDeviceCount == 0, "Not physical device with vulkan support found");
            std::vector<VkPhysicalDevice, decltype(physDevAlloc)> physicalDevices{ physDevAlloc };
            physicalDevices.resize(physicalDeviceCount);
            vkEnumeratePhysicalDevices(m_Context.Instance, &physicalDeviceCount, physicalDevices.data());

            int highestScore = -1;
            for (const auto& device : physicalDevices)
            {
                std::optional<uint32_t> graphicsQueueFamilyIndex, presentQueueFamilyIndex;
                {
                    uint32_t physicalDeviceQueueFamilyCount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, nullptr);
                    std::vector<VkQueueFamilyProperties, decltype(queueFamPropAlloc)> queueFamilyProperties{ queueFamPropAlloc };
                    queueFamilyProperties.resize(physicalDeviceQueueFamilyCount);
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
                    std::vector<VkExtensionProperties, decltype(extPropAlloc)> deviceExtensions{ extPropAlloc };
                    deviceExtensions.resize(deviceExtensionCount);
                    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExtensions.data());
                    std::unordered_set<std::string> requiredExtensions{ requiredDeviceExtensions, requiredDeviceExtensions + sizeof(requiredDeviceExtensions) / sizeof(requiredDeviceExtensions[0]) };
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
            COUST_CORE_PANIC_IF(m_Context.PhysicalDevice == VK_NULL_HANDLE, "No suitable physical device found");

            vkGetPhysicalDeviceProperties(m_Context.PhysicalDevice, m_Context.GPUProperties.get());
        }
 
        {
            std::unordered_set<uint32_t> queueFamilyIndices =
            {
                m_Context.PresentQueueFamilyIndex,
                m_Context.GraphicsQueueFamilyIndex,
                m_Context.ComputeQueueFamilyIndex,
            };
            std::vector<VkDeviceQueueCreateInfo, decltype(queueCIAlloc)> deviceQueueInfo{ queueCIAlloc };
            deviceQueueInfo.reserve(queueFamilyIndices.size());
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
                    .enabledExtensionCount = (uint32_t) sizeof(requiredDeviceExtensions) / sizeof(requiredDeviceExtensions[0]),
                    .ppEnabledExtensionNames = requiredDeviceExtensions,
                    .pEnabledFeatures = &physicalDeviceFeatures,
                };
                VK_CHECK(vkCreateDevice(m_Context.PhysicalDevice, &deviceInfo, nullptr, &m_Context.Device), "Can't create vulkan device");
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
                VK_CHECK(vmaCreateAllocator(&vmaAllocInfo, &m_Context.VmaAlloc), "Can't create VMA allocator");
            }
        }
    }

//////////////////////////////////////
/////           API              /////
//////////////////////////////////////

	void Driver::CollectGarbage() noexcept
    {
        m_StagePool.GC();
        m_FBOCache.GC();
        m_Refrigerator.GC();
        m_Context.CmdBufCacheGraphics->GC();
    }

    void Driver::BegingFrame()  noexcept
    {
    }

    void Driver::EndFrame() noexcept
    {
        // Only collect garbage when a command buffer gets submitted (next frame)
        if (m_Context.CmdBufCacheGraphics->Flush())
            CollectGarbage();
    }

    
//////////////////////////////////////
/////           API              /////
//////////////////////////////////////

//////////////////////////////////////
/////           TEST             /////
////////////////////////////////////// 
        std::unique_ptr<Image> cuteCat{ nullptr };

    void Driver::InitializationTest()
    {
        auto path = FileSystem::GetFullPathFrom({ "Coust", "asset", "orange-cat-face-pixabay.jpg" });
        int width = 0, height = 0, channel = 0;
        auto data = stbi_load(path.string().c_str(), &width, &height, &channel, STBI_rgb_alpha);
        Image::ConstructParam_Create p 
        {
            .ctx = m_Context,
            .width = (uint32_t) width,
            .height = (uint32_t) height,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage = Image::Usage::Texture2D,
            .mipLevels = 1,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .dedicatedName = "Test image",
        };
        if (!Image::Create(cuteCat, p))
            return;
        Image::UpdateParam up 
        {
            .dataFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .width = (uint32_t) width,
            .height = (uint32_t) height,
            .data = data,
            .dstImageLayer = 0,
            .dstImageLayerCount = 1,
            .dstImageMipmapLevel = 0,
        };
        cuteCat->Update(m_StagePool, up);

        m_Context.CmdBufCacheGraphics->Flush();
    }

    void Driver::LoopTest()
    {
        m_Context.CmdBufCacheGraphics->GC();
        m_StagePool.GC();
        m_FBOCache.GC();

        RenderPass::ConstructParam rp 
        {
			.ctx = m_Context,
			.depthFormat = VK_FORMAT_UNDEFINED,
			.clearMask = 0u,
			.discardStartMask = COLOR0 | DEPTH,
			.discardEndMask = COLOR0 | DEPTH,
			.sample = VK_SAMPLE_COUNT_1_BIT,
			.resolveMask = 0u,
			.inputAttachmentMask = 0u,
			.depthResolve = false,
			.dedicatedName = "Test Render Pass",
        };
        rp.colorFormat[0] = m_Swapchain.SurfaceFormat.format;
        auto r = m_FBOCache.GetRenderPass(rp);

        Framebuffer::ConstructParam fp 
        {
			.ctx = m_Context,
			.renderPass = *r,
			.width = m_Swapchain.Extent.width,
			.height = m_Swapchain.Extent.height,
			.layers = 1u,
			.depth = nullptr,
			.depthResolve = nullptr,
			.dedicatedName = "Test Framebuffer",
        }; 
        fp.color[0] = m_Swapchain.GetColorAttachment().GetSingleLayerView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0);
        auto f = m_FBOCache.GetFramebuffer(fp); (void) f;

        ShaderSource vs{ FileSystem::GetFullPathFrom({ "Coust", "shaders", "triangle.vert" })};
        ShaderModule::ConstructParm smpV
        {
            .ctx = m_Context,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .source = vs,
            .dedicatedName = "Test Vertex Shader Module",
        };
        m_GraphicsPipeCache.BindShader(smpV);

        ShaderSource fs{ FileSystem::GetFullPathFrom({ "Coust", "shaders", "triangle.frag" })};
        ShaderModule::ConstructParm smpF
        {
            .ctx = m_Context,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .source = fs,
            .dedicatedName = "Test Vertex Shader Module",
        };
        m_GraphicsPipeCache.BindShader(smpF);

        m_GraphicsPipeCache.BindShaderFinished();

        if (!m_GraphicsPipeCache.BindPipelineLayout()) return;

        GraphicsPipeline::RasterState defaultRS{};
        m_GraphicsPipeCache.BindRasterState(defaultRS);

        m_GraphicsPipeCache.BindRenderPass(r, 0);

        if (cuteCat)
        {
            SamplerCache::SamplerInfo defaultSI{};
            VkSampler sampler = m_SamplerCache.Get(defaultSI);
            m_GraphicsPipeCache.BindImage("tex1", sampler, *cuteCat, 0);
        }

        VkCommandBuffer cmdBuf = m_Context.CmdBufCacheGraphics->Get();

        m_GraphicsPipeCache.BindDescriptorSet(cmdBuf);
        m_GraphicsPipeCache.BindPipeline(cmdBuf);

        m_Context.CmdBufCacheGraphics->Flush();
    }

    void Driver::ShutdownTest()
    {
        cuteCat.reset();
    }

    // void Driver::InitializationTest()
    // {
    //     m_Context.CmdBufCacheGraphics->Flush();
    // }

    // void Driver::LoopTest()
    // {
    //     m_StagePool.GC();
    //     m_FBOCache.GC();
    //     m_Context.CmdBufCacheGraphics->GC();

    //     m_Context.CmdBufCacheGraphics->Flush();
    // }

    // void Driver::ShutdownTest()
    // {
    // }

//////////////////////////////////////
/////           TEST             /////
//////////////////////////////////////
}