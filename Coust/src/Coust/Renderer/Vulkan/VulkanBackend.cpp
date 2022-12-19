#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanBackend.h"

#include "Coust/Renderer/Vulkan/VulkanUtils.h"
#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
		VkSampleCountFlagBits g_MSAASampleCount				= VK_SAMPLE_COUNT_1_BIT;
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

		bool Backend::Commit()
		{
			if (!s_Instance)
				return false;

			return s_Instance->CommitDrawCommands();
		}

		bool Backend::RecreateSwapchainAndFramebuffers()
		{
			if (!s_Instance)
				return false;

			bool result = true;
			result = s_Instance->m_Swapchain.Recreate();
			result = s_Instance->m_FramebufferManager.RecreateFramebuffersAttachedToSwapchain();
			return result;
		}

		bool Backend::Initialize()
		{
			VK_CHECK(volkInitialize());

			bool result = true;

			result = CreateInstance() &&
#ifndef COUST_FULL_RELEASE
				CreateDebugMessengerAndReportCallback() &&
#endif
				CreateSurface() &&
				SelectPhysicalDeviceAndCreateDevice() &&
				CreateFrameSynchronizationObject() &&
				m_Swapchain.Initialize() && m_Swapchain.Create() &&
				m_PipelineManager.Initialize() &&
				m_CommandBufferManager.Initialize(FRAME_IN_FLIGHT);

			g_Swapchain = &m_Swapchain;

			g_AllVulkanGlobalVarSet = result;

			if (result)
				s_Instance = this;
			return result;
		}
		
		void Backend::Shutdown()
		{
			m_CommandBufferManager.Cleanup();

			m_DescriptorSetManager.Cleanup();

			m_RenderPassManager.Cleanup();

			m_PipelineManager.Cleanup();

            m_FramebufferManager.Cleanup();

			m_Swapchain.Cleanup();

			FlushDeletor();
		}

		/******* TEST FUNC *******/
		bool Backend::CommitDrawCommands()
		{
			static bool initialized = false;
			static VkRenderPass renderPass = VK_NULL_HANDLE;
			static std::vector<VkFramebuffer> framebuffers(g_Swapchain->m_CurrentSwapchainImageCount, VK_NULL_HANDLE);
			static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			static VkPipeline pipeline = VK_NULL_HANDLE;
			if (!initialized)
			{
				RenderPassManager::Param renderpassParam
				{
					.colorFormat = g_Swapchain->m_Format.format,
					.useColor = true,
					.useDepth = false,
					.clearColor = true,
					.clearDepth = false,
					.firstPass = true,
					.lastPass = true,
				};
				if (!renderPass && !m_RenderPassManager.CreateRenderPass(renderpassParam, &renderPass))
				{
					renderPass = VK_NULL_HANDLE;
					return false;
				}

				if (!framebuffers[0] && !m_FramebufferManager.CreateFramebuffersAttachedToSwapchain(renderPass, false, framebuffers.data()))
				{
					framebuffers = std::vector<VkFramebuffer>(g_Swapchain->m_CurrentSwapchainImageCount, VK_NULL_HANDLE);
					return false;
				}

				Utils::Param_CreatePipelineLayout pipelineLayoutParam
				{
					.descriptorSetCount = 0,
					.pDescriptorSets = nullptr,
				};
				Utils::CreatePipelineLayout(pipelineLayoutParam, &pipelineLayout);
				PipelineManager::Param pipelineParam
				{
					// .shaderFiles = { "Coust/shaders/triangle.vert", "Coust/shaders/triangle.frag" },
					.shaderFiles = { FileSystem::GetFullPathFrom({"Coust", "shaders", "triangle.vert"}), FileSystem::GetFullPathFrom({"Coust", "shaders", "triangle.frag"})}, 
					.macroes = {{}, {}},
					.useDepth = false,
					.useBlending = false,
					.layout = pipelineLayout,
					.renderpass = renderPass,
				};
				if (!pipeline && !m_PipelineManager.BuildGraphics(pipelineParam, &pipeline))
				{
					pipeline = VK_NULL_HANDLE;
					return false;
				}

				m_CommandBufferManager.AddDrawCmd([](VkCommandBuffer cmd, uint32_t swapchainImageIdx) -> bool
				{
					VkCommandBufferBeginInfo beginInfo
					{
						.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
						.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
					};
					VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

					VkClearValue clearValue[2]
					{
						{
							.color = { { 0.2f, 0.2f, 0.2f, 1.0f } },
						},
						{
							.color = { { 0.2f, 0.2f, 0.2f, 1.0f } },
						},
					};
					VkRect2D renderArea
					{
						.offset = { 0, 0 },
						.extent = g_Swapchain->m_Extent,
					};
					VkRenderPassBeginInfo renderPassInfo
					{
						.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
						.renderPass = renderPass,
						.framebuffer = framebuffers[swapchainImageIdx],
						.renderArea = renderArea,
						.clearValueCount = 2,
						.pClearValues = clearValue,
					};
					vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkViewport viewport
					{
						.x = 0.0f,
						.y = 0.0f,
						.width =  (float) g_Swapchain->m_Extent.width,
						.height = (float) g_Swapchain->m_Extent.height,
						.minDepth = 0.0f,
						.maxDepth = 1.0f,
					};
					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor
					{
						.offset = { 0, 0 },
						.extent = g_Swapchain->m_Extent,
					};
					vkCmdSetScissor(cmd, 0, 1, &scissor);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
					vkCmdDraw(cmd, 3, 1, 0, 0);

					vkCmdEndRenderPass(cmd);
					VK_CHECK(vkEndCommandBuffer(cmd));
					return true;
				});

				initialized = true;
			}

			// 1 min 
			uint64_t timeout = (uint64_t)1e9;
			VK_CHECK(vkWaitForFences(g_Device, 1, &m_RenderFence[m_FrameIndex], true, timeout));
			VK_CHECK(vkResetFences(g_Device, 1, &m_RenderFence[m_FrameIndex]));

			uint32_t swapchainImageIdx = 0;
			VK_CHECK(vkAcquireNextImageKHR(g_Device, g_Swapchain->GetHandle(), timeout, m_PresentSemaphore[m_FrameIndex], nullptr, &swapchainImageIdx));

			if (!m_CommandBufferManager.RecordDrawCmd(m_FrameIndex, swapchainImageIdx))
			{
				COUST_CORE_ERROR("Failed to record vulkan commands");
				return false;
			}

			VkCommandBuffer cmd = m_CommandBufferManager.GetCommandBuffer(m_FrameIndex);
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submitInfo
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_PresentSemaphore[m_FrameIndex],
				.pWaitDstStageMask = &waitStage,
				.commandBufferCount = 1,
				.pCommandBuffers = &cmd,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &m_RenderSemaphore[m_FrameIndex],
			};
			VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_RenderFence[m_FrameIndex]));

			VkSwapchainKHR swapchain = g_Swapchain->GetHandle();
			VkPresentInfoKHR presentInfo
			{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_RenderSemaphore[m_FrameIndex],
				.swapchainCount = 1,
				.pSwapchains = &swapchain,
				.pImageIndices = &swapchainImageIdx,
			};
			VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

			m_FrameIndex = (m_FrameIndex + 1) % FRAME_IN_FLIGHT;

			return true;
		}
		/******* TEST FUNC *******/
		
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
					// Get MSAA sample count
					{
						VkSampleCountFlags MSAACountFlags = physicalDeviceProperties.limits.framebufferColorSampleCounts & 
							physicalDeviceProperties.limits.framebufferDepthSampleCounts;
						if (MSAACountFlags & VK_SAMPLE_COUNT_64_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_64_BIT;
						else if (MSAACountFlags & VK_SAMPLE_COUNT_32_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_32_BIT;
						else if (MSAACountFlags & VK_SAMPLE_COUNT_16_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_16_BIT;
						else if (MSAACountFlags & VK_SAMPLE_COUNT_8_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_8_BIT;
						else if (MSAACountFlags & VK_SAMPLE_COUNT_4_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_4_BIT;
						else if (MSAACountFlags & VK_SAMPLE_COUNT_2_BIT)
							m_MSAASampleCount = VK_SAMPLE_COUNT_2_BIT;
						
						g_MSAASampleCount = m_MSAASampleCount;
					}
					// Get MSAA sample count
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

		bool Backend::CreateFrameSynchronizationObject()
		{
			bool result = true;
			for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i)
			{
				result = Utils::CreateFence(true, &m_RenderFence[i]);
				result = Utils::CreateSemaphores(&m_RenderSemaphore[i]);
				result = Utils::CreateSemaphores(&m_PresentSemaphore[i]);
				AddDeletor([=]()
				{
					vkDestroyFence(m_Device, m_RenderFence[i], nullptr);
					vkDestroySemaphore(m_Device, m_RenderSemaphore[i], nullptr);
					vkDestroySemaphore(m_Device, m_PresentSemaphore[i], nullptr);
				});
			}

			return result;
		}
	}
}

