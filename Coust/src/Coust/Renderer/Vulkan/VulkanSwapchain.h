#pragma once

#include <volk.h>

#include "Coust/Renderer/Vulkan/VulkanStructs.h"

#include <vector>

namespace Coust
{
    namespace VK
    {
		struct ImageAlloc;

		class Swapchain
		{
		public:
			Swapchain() = default;
			~Swapchain() = default;
			Swapchain(const Swapchain&) = delete;
			Swapchain& operator=(const Swapchain&) = delete;

			bool Initialize();

			void Cleanup();

			bool Create();

			bool Recreate();

			VkSwapchainKHR GetHandle() const { return m_Swapchain; }

			VkImageView GetDepthImageView() const { return m_DepthImageView; }

			const std::vector<VkImageView>& GetColorImageViews() const { return m_ImageViews; }

		public:
			VkExtent2D m_Extent{};
			VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
			uint32_t m_MinImageCount = 0;
			VkSurfaceFormatKHR m_Format{};
			VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

			uint32_t m_CurrentSwapchainImageCount = 0;
            // in case the number of swapchain image changes...
            uint32_t m_OldSwapchainImageCount = 0;

		private:
			bool m_Recreation = false;

			VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
			std::vector<VkImage> m_Images{};
			std::vector<VkImageView> m_ImageViews{};

			ImageAlloc m_DepthImageAlloc{};
			VkImageView m_DepthImageView = VK_NULL_HANDLE;

		};
    }
}
