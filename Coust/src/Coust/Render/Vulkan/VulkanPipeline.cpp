#include "pch.h"

#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"

namespace Coust::Render::VK
{
	PipelineLayout::PipelineLayout(const ConstructParam& param)
		: Base(param.ctx, VK_NULL_HANDLE),
		  Hashable(param.GetHash()),
		  m_ShaderModules(param.shaderModules)
	{
		ShaderModule::CollectShaderResources(param.shaderModules, m_ShaderResources, m_SetToResourceIdxLookup);

		for (auto& pair : m_SetToResourceIdxLookup)
		{
			uint32_t set = pair.first;
			std::vector<ShaderResource> resources{};
			for (uint64_t i = 0; i < sizeof(pair.second); ++ i)
			{
				if (((uint64_t(1) << i) & pair.second) != 0)
					resources.push_back(m_ShaderResources[i]);
			}
			
			std::string debugName{};
			for (const auto& res : resources)
			{
				debugName += res.Name;
				debugName += ' ';
			}
			debugName += std::to_string(set);

			DescriptorSetLayout::ConstructParam p 
			{
            	.ctx = param.ctx,
            	.set = set,
            	.shaderModules = param.shaderModules,
            	.shaderResources = resources,
            	.dedicatedName = debugName.c_str(),
			};
			m_DescriptorLayouts.emplace_back(p);

			if (!DescriptorSetLayout::CheckValidation(m_DescriptorLayouts.back()))
			{
				COUST_CORE_ERROR("Can't create descriptor set layout for shader resources: {}", debugName);
				m_DescriptorLayouts.pop_back();
				return;
			}
		}
		std::vector<VkDescriptorSetLayout> setLayouts{};
		setLayouts.reserve(m_DescriptorLayouts.size());
		for (const auto& l : m_DescriptorLayouts)
		{
			setLayouts.push_back(l.GetHandle());
		}

    	std::vector<VkPushConstantRange> pushConstantRanges{};
		for (const auto& res : m_ShaderResources)
		{
			if (res.Type == ShaderResourceType::PushConstant)
			{
				VkPushConstantRange r 
				{
    				.stageFlags = res.Stage,
    				.offset = res.Offset,
    				.size = res.Size,
				};
				pushConstantRanges.push_back(r);
			}
		}

		VkPipelineLayoutCreateInfo CI 
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    		.setLayoutCount = (uint32_t) m_DescriptorLayouts.size(),
    		.pSetLayouts = setLayouts.data(),
    		.pushConstantRangeCount = (uint32_t) pushConstantRanges.size(),
    		.pPushConstantRanges = pushConstantRanges.data(),
		};
		bool success = false;
		VK_REPORT(vkCreatePipelineLayout(m_Ctx.Device, &CI, nullptr, &m_Handle), success);
		if (success)
		{
			if (param.dedicatedName)
				SetDedicatedDebugName(param.dedicatedName);
			else if (param.scopeName)
				SetDefaultDebugName(param.scopeName, nullptr);
		}
		else  
			m_Handle = VK_NULL_HANDLE;
	}

	PipelineLayout::PipelineLayout(PipelineLayout&& other)
		: Base(std::forward<Base>(other)),
		  Hashable(std::forward<Hashable>(other)),
          m_ShaderModules(std::move(other.m_ShaderModules)),
		  m_DescriptorLayouts(std::move(other.m_DescriptorLayouts)),
		  m_ShaderResources(std::move(other.m_ShaderResources)),
		  m_SetToResourceIdxLookup(std::move(other.m_SetToResourceIdxLookup))
	{}

	PipelineLayout::~PipelineLayout()
	{
		if (m_Handle != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_Ctx.Device, m_Handle, nullptr);
	}

	const std::vector<ShaderModule*>& PipelineLayout::GetShaderModules() const { return m_ShaderModules; }

	const std::vector<DescriptorSetLayout>& PipelineLayout::GetDescriptorSetLayouts() const { return m_DescriptorLayouts; }

	const std::vector<ShaderResource>& PipelineLayout::GetShaderResources() const { return m_ShaderResources; }

	// GraphicsPipeline::GraphicsPipeline(const ConstructParam& param)
	// 	: Base(param.ctx, VK_NULL_HANDLE),
	// 	  Hashable(param.GetHash())
	// {
	// 
	// }

	// GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other);

	// GraphicsPipeline::~GraphicsPipeline();

	size_t PipelineLayout::ConstructParam::GetHash() const 
	{
		size_t h = 0;
		return h;
	}

	size_t GraphicsPipeline::ConstructParam::GetHash() const 
	{
		size_t h = 0;
		return h;
	}
}
