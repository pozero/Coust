#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanBackend.h"

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

#include <GLFW/glfw3.h>

namespace Coust
{
	extern GLFWwindow* g_WindowHandle;
	namespace VK
	{
		VkInstance g_Instance								= VK_NULL_HANDLE;
		VkSurfaceKHR g_Surface								= VK_NULL_HANDLE;
		VkPhysicalDevice g_PhysicalDevice					= VK_NULL_HANDLE;
		VkPhysicalDeviceProperties* g_pPhysicalDevProps		= VK_NULL_HANDLE;
		VkDevice g_Device									= VK_NULL_HANDLE;
		VkQueue g_GraphicsQueue								= VK_NULL_HANDLE;
		VmaAllocator g_VmaAlloc								= VK_NULL_HANDLE;
		uint32_t g_GraphicsQueueFamilyIndex					= 0;
		uint32_t g_PresentQueueFamilyIndex					= 0;
		const Swapchain* g_Swapchain						= nullptr;
		bool g_AllVulkanGlobalVarSet						= false;

		Backend* Backend::s_Instance = nullptr;

		bool Backend::Init()
		{
            if (s_Instance)
            {
			    COUST_CORE_ERROR("Vulkan backend already initialized");
                return false;
            }

			s_Instance = new Backend();
			return s_Instance->Initialize();
		}

		void Backend::Shut()
		{
			if (s_Instance)
			{
				s_Instance->Shutdown();
				s_Instance = nullptr;
			}
		}

		bool Backend::Initialize()
		{
			VK_CHECK(volkInitialize());

			bool result = true;

			result =	CreateInstance() &&
#ifndef COUST_FULL_RELEASE
						CreateDebugMessengerAndReportCallback() &&
#endif
						CreateSurface() &&
						SelectPhysicalDeviceAndCreateDevice() &&
						m_Swapchain.Initialize() && m_Swapchain.Create();

			g_AllVulkanGlobalVarSet = true;

			if (result)
				s_Instance = this;
			return result;
		}
		
		void Backend::Shutdown()
		{
			m_Swapchain.Cleanup();

			FlushDeletor();
		}
		
		bool Backend::CreateInstance()
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
    		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = Utils::DebugMessengerCreateInfo();
      		instanceInfo.pNext                       = (void*)&debugMessengerCreateInfo;
#endif
	
			VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));
			AddDeletor([=]() { vkDestroyInstance(m_Instance, nullptr); });
			g_Instance = m_Instance;
	
			volkLoadInstance(m_Instance);
			return true;
		}


		
#ifndef COUST_FULL_RELEASE
		bool Backend::CreateDebugMessengerAndReportCallback()
		{
			{
				VkDebugUtilsMessengerCreateInfoEXT info = Utils::DebugMessengerCreateInfo();
    			VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_Instance, &info, nullptr, &m_DebugMessenger));
			}

			{
				VkDebugReportCallbackCreateInfoEXT info = Utils::DebugReportCallbackCreateInfo();
				VK_CHECK(vkCreateDebugReportCallbackEXT(m_Instance, &info, nullptr, &m_DebugReportCallback));
			}

			AddDeletor([=]()
			{
				vkDestroyDebugReportCallbackEXT(m_Instance, m_DebugReportCallback, nullptr);
				vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr); 
			});
			return true;
		}
#endif
		
		bool Backend::CreateSurface()
		{
			VK_CHECK(glfwCreateWindowSurface(m_Instance, g_WindowHandle, nullptr, &m_Surface));
			AddDeletor([=]() { vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); });
			g_Surface = m_Surface;
			return true;
		}

		
		bool Backend::SelectPhysicalDeviceAndCreateDevice()
		{
			std::vector <const char*> requiredDeviceExtensions
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			{
				uint32_t physicalDeviceCount = 0;
				vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);
				if (physicalDeviceCount == 0)
				{
					COUST_CORE_ERROR("Not physical device with vulkan support found");
					return false;
				}
				std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
				vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices.data());

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
							vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &supportPresent);
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
					if (requiredExtensionSupported && requiredFeatureSupported && presentQueueFamilyIndex.has_value() && graphicsQueueFamilyIndex.has_value())
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
							m_PhysicalDevice = device;
							m_PresentQueueFamilyIndex = presentQueueFamilyIndex.value();
							m_GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex.value();
						}
					}
				}
				if (m_PhysicalDevice == VK_NULL_HANDLE)
				{
					COUST_CORE_ERROR("No suitable physical device found");
					return false;
				}

				vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDevProps);

				g_PhysicalDevice = m_PhysicalDevice;
				g_pPhysicalDevProps = &m_PhysicalDevProps;
			}
		
			{
				std::unordered_set<uint32_t> queueFamilyIndices =
				{
					m_PresentQueueFamilyIndex,
					m_GraphicsQueueFamilyIndex,
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
					VkDeviceCreateInfo deviceInfo
					{
						.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
						.pNext = &shaderFeatures,
						.queueCreateInfoCount = (uint32_t)deviceQueueInfo.size(),
						.pQueueCreateInfos = deviceQueueInfo.data(),
						.enabledLayerCount = 0,
						.enabledExtensionCount = (uint32_t)requiredDeviceExtensions.size(),
						.ppEnabledExtensionNames = requiredDeviceExtensions.data(),
						.pEnabledFeatures = &physicalDeviceFeatures,
					};
					VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_Device));
					AddDeletor([=]() { vkDestroyDevice(m_Device, nullptr); });
					g_Device = m_Device;
				}

				volkLoadDevice(m_Device);

				vkGetDeviceQueue(m_Device, m_PresentQueueFamilyIndex, 0, &m_PresentQueue);
				vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
				g_GraphicsQueue = m_GraphicsQueue;

				{
					VmaVulkanFunctions functions
					{
						.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
						.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
						// .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
						// .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
						// .vkAllocateMemory = vkAllocateMemory,
						// .vkFreeMemory = vkFreeMemory,
						// .vkMapMemory = vkMapMemory,
						// .vkUnmapMemory = vkUnmapMemory,
						// .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
						// .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
						// .vkBindBufferMemory = vkBindBufferMemory,
						// .vkBindImageMemory = vkBindImageMemory,
						// .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
						// .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
						// .vkCreateBuffer = vkCreateBuffer,
						// .vkDestroyBuffer = vkDestroyBuffer,
						// .vkCreateImage = vkCreateImage,
						// .vkDestroyImage = vkDestroyImage,
						// .vkCmdCopyBuffer = vkCmdCopyBuffer,
						// .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
						// .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
						// .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
						// .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
						// .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
						// .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
						// .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements,
					};
					VmaAllocatorCreateInfo vmaAllocInfo 
					{
						.physicalDevice		= m_PhysicalDevice,
						.device				= m_Device,
						.pVulkanFunctions	= &functions,
						.instance			= m_Instance,
						.vulkanApiVersion	= VULKAN_API_VERSION,
					};
					VK_CHECK(vmaCreateAllocator(&vmaAllocInfo, &m_VmaAlloc));
					AddDeletor([=]() { vmaDestroyAllocator(m_VmaAlloc); });
					g_VmaAlloc = m_VmaAlloc;
				}
			}
			return true;
		}

		void Backend::AddFramebufferRecreator(FramebufferRecreateFn&& fn)
		{
			s_Instance->m_FramebufferRecreators.push_back(fn);
		}

		bool Backend::RecreateSwapchainAndFramebuffers()
		{
			bool result = s_Instance->m_Swapchain.Recreate();
			if (result)
			{
				uint32_t width = s_Instance->m_Swapchain.m_Extent.width;
				uint32_t height = s_Instance->m_Swapchain.m_Extent.height;
				const std::vector<VkImageView>& colorImageViews = s_Instance->m_Swapchain.GetColorImageViews();
				VkImageView depthImageView = s_Instance->m_Swapchain.GetDepthImageView();
				for (const auto& recreator : s_Instance->m_FramebufferRecreators)
				{
					result = recreator(width, height, colorImageViews, depthImageView);
				}
			}
			return result;
		}
	}
}

