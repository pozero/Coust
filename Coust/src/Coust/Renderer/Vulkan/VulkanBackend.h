#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <functional>

#include "Coust/Renderer/Vulkan/VulkanRenderPass.h"
#include "Coust/Renderer/Vulkan/VulkanDescriptorSet.h"
#include "Coust/Renderer/Vulkan/VulkanSwapchain.h"
#include "Coust/Renderer/Vulkan/VulkanFramebuffer.h"
#include "Coust/Renderer/Vulkan/VulkanPipeline.h"	
#include "Coust/Renderer/Vulkan/VulkanCommandBuffer.h"
#include "vulkan/vulkan_core.h"

namespace Coust
{
	namespace VK
	{
		constexpr uint32_t FRAME_IN_FLIGHT = 2;

		class Backend
		{
		public:
			static bool Init();
			static void Shut();

			static bool Commit();

			static bool RecreateSwapchainAndFramebuffers();

		private:
			static Backend* s_Instance;

		private:
			bool Initialize();
			void Shutdown();

			bool CommitDrawCommands();

		private:
			bool CreateInstance();

#ifndef COUST_FULL_RELEASE
			bool CreateDebugMessengerAndReportCallback();
#endif

			bool CreateSurface();

			bool SelectPhysicalDeviceAndCreateDevice();

		private:
			uint32_t m_FrameIndex = 0;
			VkSemaphore m_RenderSemaphore[FRAME_IN_FLIGHT]{};
			VkSemaphore m_PresentSemaphore[FRAME_IN_FLIGHT]{};
			VkFence m_RenderFence[FRAME_IN_FLIGHT]{};

			bool CreateFrameSynchronizationObject();

		public:
			Swapchain m_Swapchain{};

			FramebufferManager m_FramebufferManager{};

			PipelineManager m_PipelineManager{};

			DesriptorSetManager m_DescriptorSetManager{};
			
			RenderPassManager m_RenderPassManager{};

			CommandBufferManager m_CommandBufferManager{};

		private:
			VmaAllocator m_VmaAlloc = nullptr;

			VkInstance m_Instance = VK_NULL_HANDLE;

			VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
			VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;

			VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

			VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

			VkDevice m_Device = VK_NULL_HANDLE;

			uint32_t m_PresentQueueFamilyIndex = -1;
			uint32_t m_GraphicsQueueFamilyIndex = -1;

			VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
			VkQueue m_PresentQueue = VK_NULL_HANDLE;

			VkPhysicalDeviceProperties m_PhysicalDevProps{};

			VkSampleCountFlagBits m_MSAASampleCount = VK_SAMPLE_COUNT_1_BIT;

		private:
			void AddDeletor(std::function<void()>&& f)
			{
				m_Deletor.push_back(f);
			}

			void FlushDeletor()
			{
				for (auto iter = m_Deletor.rbegin(); iter != m_Deletor.rend(); ++iter)
				{
					(*iter)();
				}
			}

		private:
			std::vector<std::function<void()>> m_Deletor{};
		};
	}
}
