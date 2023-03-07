#include "pch.h"

#include "Coust/Core/Window.h"
#include "Coust/Render/Vulkan/VulkanDriver.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

#include "Coust/Utils/FileSystem.h"

#include <GLFW/glfw3.h>

namespace Coust::Render::VK
{
    Driver::Driver()
    {
	    VK_REPORT(volkInitialize(), m_IsInitialized);

		m_IsInitialized = CreateInstance() &&
#ifndef COUST_FULL_RELEASE
			CreateDebugMessengerAndReportCallback() &&
#endif
			CreateSurface() &&
			SelectPhysicalDeviceAndCreateDevice();
    }

    Driver::~Driver()
    {
		m_IsInitialized = false;
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
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

			vkGetPhysicalDeviceProperties(m_Context.PhysicalDevice, &m_Context.PhysicalDevProps);
		}
 
		{
			std::unordered_set<uint32_t> queueFamilyIndices =
			{
				m_Context.PresentQueueFamilyIndex,
				m_Context.GraphicsQueueFamilyIndex,
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
				VkDeviceCreateInfo deviceInfo
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
					.pNext = &physicalDeviceDescriptorIndexingFeatures,
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
		Image::ConstructParam p
		{
            .ctx = m_Context,
			.extent = 
			{
				.width = 1200,
				.height = 800,
				.depth = 1,
			},
            .format = VK_FORMAT_R8G8B8A8_SNORM,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .flags = 0,
            .mipLevels = 5,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .relatedQueues = nullptr,
            .dedicatedName = "Test",
		};
		Image* i = nullptr;
		COUST_CORE_INFO(Image::Base::Create(i, p));

		ImageView::ConstructParam p1
		{
            .ctx = m_Context,
            .image = *i,
            .type = VK_IMAGE_VIEW_TYPE_2D,
            .format  = VK_FORMAT_UNDEFINED,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
            .dedicatedName  = "Test",
		};
		ImageView* iv = nullptr;
		COUST_CORE_INFO(ImageView::Base::Create(iv, p1));

		delete i;
		delete iv;
	}
}