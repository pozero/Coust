#pragma once

#include "Coust/Render/Driver.h"
#include "Coust/Render/Vulkan/StagePool.h"
#include "Coust/Render/Vulkan/Refrigerator.h"
#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Vulkan/VulkanSampler.h"

namespace Coust::Render::VK
{
    class Driver : public Coust::Render::Driver
    {
	public:
		explicit Driver();
		virtual ~Driver();
		
		virtual void InitializationTest() override;

		virtual void LoopTest() override;

		void ShutdownTest();

	public:
		const Context& GetContext() const { return m_Context; }

	public:

	// Driver API (It's vulkan specific now, they'll be abstracted when we decide to add a new graphics API)

		// collect and recycle unused cache, it'll be implicitly called per submission
		void CollectGarbage() noexcept;

		void BegingFrame();

		void EndFrame();

	/*
		void CreateRenderTarget();
		void DestroyRenderTarget();

		void SetRenderTarget();

		void CreateRenderPrimitive();
		void DestroyRenderPrimitive();

		void CreateVertexBuffer();
		void DestroyVertexBuffer();

		// vertex buffer is a set of buffers
		void SetVertexBuffer();

		// index buffer is basically a buffer
		void CreateIndexBuffer();
		void DestroyIndexBuffer();

		void UpdateIndexBuffer();

		void CreateBuffer();
		void DestroyBuffer();

		void UpdateBuffer();

		void CreateTexture();
		void DestroyTexture();

		void SetTextureMipMapLevel();

		void CreateProgram();
		void DestroyProgram();

		void BeginRenderPass();
		void EndRenderPass();

		void NextSubPass();

		void Present();

		void BindBuffer();
		// maybe we need unbind?

		void BindTexture();

		void ReadPixels();

		void BlitImage();
	
	*/
		
	private:
		bool CreateInstance();

		bool CreateDebugMessengerAndReportCallback();

		bool CreateSurface();

		bool SelectPhysicalDeviceAndCreateDevice();

    private:
        Context m_Context{};

		StagePool m_StagePool;

		Swapchain m_Swapchain;

		FBOCache m_FBOCache;

		GraphicsPipelineCache m_GraphicsPipeCache;

		SamplerCache m_SamplerCache;

		Refrigerator m_Refrigerator;
    };
}