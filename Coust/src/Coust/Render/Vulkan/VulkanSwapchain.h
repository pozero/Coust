#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <vector>

namespace Coust::Render::VK
{
	/**
	 * @brief Simple wrapper class containing `VkSwapchainKHR` and its images `VkImage`.
	 * 		  Responsible for creating or recreating swapchain.
	 */
	class Swapchain : public Resource<VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR>
	{
	public:
		using Base = Resource<VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR>;

	public:
		/**
		 * @brief Get appropriate parameter and create swapchain using it.
		 * @param ctx 
		 */
		Swapchain(const Context &ctx);

		~Swapchain();

		Swapchain() = delete;

		// copy & move prohibited
		Swapchain(Swapchain&&) = delete;
		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;

		bool Recreate(const Context &ctx);
		
		VkResult AcquireNextImage(uint64_t timeOut, VkSemaphore semaphoreToSignal, VkFence fenceToSignal, uint32_t* out_ImageIndex);

		bool IsValid() const { return m_IsValid; }

		VkSurfaceFormatKHR GetFormat() const;

		VkPresentModeKHR GetPresentMode() const;

		uint32_t GetMinImageCount() const;

		VkExtent2D GetExtent() const;

		VkFormat GetDepthFormat() const;

		uint32_t GetCurrentSwapchainImageCount() const;
	
	private:
		bool Create(const Context &ctx);

	public:
		// We pass Swapchain through `const Swapchain&` most of the time, 
		// so declaring these properties as public members is just fine.

		VkSurfaceFormatKHR Format{};

		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t MinImageCount = 0;

		VkExtent2D Extent{};

		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

		uint32_t CurrentSwapchainImageCount = 0;

	private:
		bool m_IsValid = false;

		std::vector<VkImage> m_Images{};
	};
}
