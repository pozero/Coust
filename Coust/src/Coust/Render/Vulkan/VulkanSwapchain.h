#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

#include <vector>

namespace Coust::Render::VK
{
	class Swapchain : public Resource<VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR>
	{
	public:
		using Base = Resource<VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR>;

		Swapchain() = delete;
		Swapchain(Swapchain&&) = delete;
		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;

	public:
		explicit Swapchain(const Context &ctx) noexcept;

		~Swapchain() = default;

		void Prepare();

		bool Create();

		void Destroy() noexcept;

		// acquire next swapchain image
		bool Acquire() noexcept;

		// Manually query the change
		bool HasResized() const noexcept;

		// TODO: we let swapchain responsible for presentable layout transition for now 
		// (it should be much slower than transition inside renderpass), it will be changed later
		void MakePresentable();

		Image& GetColorAttachment() const noexcept;
		Image& GetDepthAttachment() const noexcept;
		uint32_t GetImageIndex() const noexcept;

	public:
		// We pass Swapchain through `const Swapchain&` most of the time, 
		// so declaring these properties as public members is just fine.

		VkSurfaceFormatKHR SurfaceFormat{};

		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t MinImageCount = 0;

		VkExtent2D Extent{};

		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

		VkSemaphore ImgAvaiSignal = VK_NULL_HANDLE;
		mutable bool IsNextImgAcquired = false;

		mutable bool IsFirstRenderPass = false;

		bool IsSubOptimal = false;

	private:
		std::vector<std::unique_ptr<Image>> m_Images;

		std::unique_ptr<Image> m_Depth;

		uint32_t m_CurImgIdx = 0;
	};
}
