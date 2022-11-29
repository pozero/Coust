#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <functional>

#include "Coust/Renderer/Vulkan/VulkanSwapchain.h"

namespace Coust
{
	namespace VK
	{
		constexpr uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_2;

		class Backend
		{
		public:
			static bool Init();
			static void Shut();

		private:
			static Backend* s_Instance;

		private:
			bool Initialize();
			void Shutdown();

		private:
			bool CreateInstance();

#ifndef COUST_FULL_RELEASE
			bool CreateDebugMessengerAndReportCallback();
#endif

			bool CreateSurface();

			bool SelectPhysicalDeviceAndCreateDevice();

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

		public:
			using FramebufferRecreateFn = std::function<bool(uint32_t width, uint32_t height, const std::vector<VkImageView>& colorImageViews, VkImageView depthImageView)>;

		public:
			static bool RecreateSwapchainAndFramebuffers();

			static void AddFramebufferRecreator(FramebufferRecreateFn&& fn);

		private:
			Swapchain m_Swapchain{};

			std::vector<FramebufferRecreateFn> m_FramebufferRecreators{};

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

		const Backend* GetBackend();
	}
}
