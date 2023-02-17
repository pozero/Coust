#pragma once

#include <functional>

#include "Coust/Render/Vulkan/VulkanUtils.h"

#include "Coust/Render/Driver.h"

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanDescriptorSet.h"
#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanFramebuffer.h"
#include "Coust/Render/Vulkan/VulkanPipeline.h"	
#include "Coust/Render/Vulkan/VulkanCommandBuffer.h"

namespace Coust::Render::VK
{
	constexpr uint32_t FRAME_IN_FLIGHT = 2;

    class Driver : public Coust::Render::Driver
    {
	public:
		bool Initialize() override;
		void Shutdown() override;

        bool RecreateSwapchainAndFramebuffers() override;

		bool FlushCommand() override;
	
	public:
		const Context& GetContext() const { return m_Context; }

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

    private:
        Context m_Context{};

		Swapchain m_Swapchain{};

		FramebufferManager m_FramebufferManager{};

		PipelineManager m_PipelineManager{};

		DesriptorSetManager m_DescriptorSetManager{};
		
		RenderPassManager m_RenderPassManager{};

		CommandBufferManager m_CommandBufferManager{};

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