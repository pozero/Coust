#pragma once

#include <volk.h>
#include "vk_mem_alloc.h"

#include <vector>
#include <functional>

namespace Coust
{
	namespace VK
	{
		constexpr uint32_t FRAME_IN_FLIGHT = 2;

		class Backend
		{
		public:
			void Initialize();
			void Shutdown();

		private:
			void CreateInstance();

#ifndef COUST_FULL_RELEASE
			void CreateDebugMessenger();
#endif

			void CreateSurface();

			void SelectPhysicalDeviceAndCreateDevice();

			void CreateSwapchain();

			void CreateFramebuffers();

			void CreateCommandObj();

			void CreateSyncObj();

		private:
			void CleanupSwapchain();

			void RecreateSwapchain();

		private:
			VmaAllocator m_VmaAlloc;

			VkInstance m_Instance;

			VkDebugUtilsMessengerEXT m_DebugMessenger;

			VkSurfaceKHR m_Surface;

			VkPhysicalDevice m_PhysicalDevice;

			VkDevice m_Device;

			uint32_t m_PresentQueueFamilyIndex;
			uint32_t m_GraphicsQueueFamilyIndex;

			VkQueue m_GraphicsQueue;
			VkQueue m_PresentQueue;

			VkPhysicalDeviceProperties m_PhysicalDevProps;


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