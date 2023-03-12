#include "pch.h"

#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

#include <stdint.h>

namespace Coust::Render::VK
{
	void PipelineState::Reset()
	{
		m_InputAssembleState = InputAssemblyState{};
		
		m_RasterizationState = RasterizationState{};
		
		m_MultisampleState = MultisampleState{};
		
		m_DepthStencilState = DepthStencilState{};
		
		m_ColorBlendState = ColorBlendState{};

		m_PipelineLayout = nullptr;
		
		m_RenderPass = nullptr;
		
		m_SubpassIndex = 0;
	}
	
	void PipelineState::SetInputAssemblyState(const InputAssemblyState& inputAssembleState)
	{
		if (m_InputAssembleState != inputAssembleState)
		{
			m_InputAssembleState = inputAssembleState;
			m_Dirty = true;
		}
	}
	
	void PipelineState::SetRasterizationState(const RasterizationState& rasterizationState)
	{
		if (m_RasterizationState != rasterizationState)
		{
			m_RasterizationState = rasterizationState;
			m_Dirty = true;
		}
	}
		
	
	void PipelineState::SetMultisampleState(const MultisampleState& multisampleState)
	{
		if (m_MultisampleState != multisampleState)
		{
			m_MultisampleState = multisampleState;
			m_Dirty = true;
		}
	}
	
	void PipelineState::SetDepthStencilState(const DepthStencilState& depthStencilState)
	{
		if (m_DepthStencilState != depthStencilState)
		{
			m_DepthStencilState = depthStencilState;
			m_Dirty = true;
		}
	}
	
	void PipelineState::SetColorBlendState(const ColorBlendState& colorBlendState)
	{
		if (m_ColorBlendState != colorBlendState)
		{
			m_ColorBlendState = colorBlendState;
			m_Dirty = true;
		}
	}

	void PipelineState::SetPipelineLayout(const PipelineLayout& pipelineLayout)
	{
		if (m_PipelineLayout && m_PipelineLayout->GetHandle() != pipelineLayout.GetHandle())
		{
			m_PipelineLayout = &pipelineLayout;
			m_Dirty = true;
		}
		else // `m_PipelineLayout` is nullptr
		{
			m_PipelineLayout = &pipelineLayout;
			m_Dirty = true;
		}
	}
	
	void PipelineState::SetRenderPass(const RenderPass& renderPass)
	{
		if (m_RenderPass && m_RenderPass->GetHandle() != renderPass.GetHandle())
		{
			m_RenderPass = &renderPass;
			m_Dirty = true;
		}
		else // `m_RenderPass` is nullptr
		{
			m_RenderPass = &renderPass;
			m_Dirty = true;
		}
	}
	
	void PipelineState::SetSubpass(const uint32_t subpassIndex)
	{
		if (m_SubpassIndex != subpassIndex)
		{
			m_SubpassIndex = subpassIndex;
			m_Dirty = true;
		}
	}

	const InputAssemblyState& PipelineState::GetInputAssemblyState() const { return m_InputAssembleState; }
	
	const RasterizationState& PipelineState::GetRasterizationState() const { return m_RasterizationState; }
	
	const MultisampleState& PipelineState::GetMultisampleState() const { return m_MultisampleState; }
	
	const DepthStencilState& PipelineState::GetDepthStencilState() const { return m_DepthStencilState; }
	
	const ColorBlendState& PipelineState::GetColorBlendState() const { return m_ColorBlendState; }
	
	const PipelineLayout* PipelineState::GetPipelineLayout() const { return m_PipelineLayout; }
	
	const RenderPass* PipelineState::GetRenderPass() const { return m_RenderPass; }
	
	const uint32_t PipelineState::GetSubpassIndex() const { return m_SubpassIndex; }

	bool PipelineState::IsDirty() const { return m_Dirty; }

	void PipelineState::Flush() { m_Dirty = false; }
}
