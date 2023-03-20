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

	PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
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

	const std::vector<ShaderModule*>& PipelineLayout::GetShaderModules() const noexcept { return m_ShaderModules; }

	const std::vector<DescriptorSetLayout>& PipelineLayout::GetDescriptorSetLayouts() const noexcept { return m_DescriptorLayouts; }

	const std::vector<ShaderResource>& PipelineLayout::GetShaderResources() const noexcept { return m_ShaderResources; }

	VkSpecializationInfo SpecializationConstantInfo::Get() const noexcept
	{
		return VkSpecializationInfo
		{
    		.mapEntryCount = (uint32_t) m_Entry.size(),
    		.pMapEntries = m_Entry.data(),
    		.dataSize = (uint32_t) m_Data.size(),
    		.pData = m_Data.data(),
		};
	}

	size_t SpecializationConstantInfo::GetHash() const noexcept
	{
		size_t h = 0;
		for (const auto& e : m_Entry)
		{
			Hash::Combine(h, e);
		}
		for (uint8_t byte : m_Data)
		{
			Hash::Combine(h, byte);
		}
		return h;
	}

	GraphicsPipeline::GraphicsPipeline(const ConstructParam& param)
		: Base(param.ctx, VK_NULL_HANDLE),
		  Hashable(param.GetHash()),
		  m_Layout(param.layout),
		  m_RenderPass(param.renderPass),
		  m_SubpassIdx(param.subpassIdx)
	{
		bool noFragmentShader = true;
		std::vector<VkPipelineShaderStageCreateInfo> stageInfos{};
		VkSpecializationInfo specializationConstantInfo = param.specializationConstantInfo ? 
			param.specializationConstantInfo->Get() :
			VkSpecializationInfo{};
		VkSpecializationInfo* pSpecializationInfo = param.specializationConstantInfo ? 
			&specializationConstantInfo :
			nullptr;
		for (const auto& s : param.layout.GetShaderModules())
		{
			if (s->GetStage() == VK_SHADER_STAGE_FRAGMENT_BIT)
				noFragmentShader = false;

			stageInfos.push_back(
				VkPipelineShaderStageCreateInfo
				{
    				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    				.stage = s->GetStage(),
    				.module = s->GetHandle(),
    				.pName = "main",
    				.pSpecializationInfo = pSpecializationInfo,
				});
		}

		std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
		ShaderModule::CollectShaderInputs(param.layout.GetShaderResources(), 0u, vertexBindingDescriptions, vertexAttributeDescriptions);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    		.vertexBindingDescriptionCount = (uint32_t) vertexBindingDescriptions.size(),
    		.pVertexBindingDescriptions = vertexBindingDescriptions.data(),
    		.vertexAttributeDescriptionCount = (uint32_t) vertexAttributeDescriptions.size(),
    		.pVertexAttributeDescriptions = vertexAttributeDescriptions.data(),
		};

    	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    		.topology = param.topology,
    		.primitiveRestartEnable = VK_FALSE,
		};

    	VkPipelineTessellationStateCreateInfo tessellationState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    		.patchControlPoints = 0,
		};

    	VkPipelineViewportStateCreateInfo viewportState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    		.viewportCount = 1,
			// The spec says:
			// pViewports is a pointer to an array of VkViewport structures, defining the viewport transforms.
			// If the viewport state is dynamic, this member is ignored.
    		.pViewports = nullptr,
    		.scissorCount = 1,
    		.pScissors = nullptr,
		};

    	VkPipelineRasterizationStateCreateInfo rasterizationState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    		.depthClampEnable = VK_FALSE,
    		.rasterizerDiscardEnable = VK_FALSE,
    		.polygonMode = param.polygonMode,
    		.cullMode = param.cullMode,
    		.frontFace = param.frontFace,
    		.depthBiasEnable = param.depthBiasEnable,
    		.depthBiasConstantFactor = param.depthBiasConstantFactor,
    		.depthBiasClamp = param.depthBiasClamp,
    		.depthBiasSlopeFactor = param.depthBiasSlopeFactor,
    		.lineWidth = 1.0f,
		};

    	VkPipelineMultisampleStateCreateInfo multisampleState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    		.rasterizationSamples = param.rasterizationSamples,
    		.sampleShadingEnable = VK_FALSE,
    		.minSampleShading = 0.0f,
    		.pSampleMask = nullptr,
    		.alphaToCoverageEnable = VK_FALSE,
    		.alphaToOneEnable = VK_FALSE,
		};

    	VkPipelineDepthStencilStateCreateInfo depthStencilState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    		.depthTestEnable = VK_TRUE,
    		.depthWriteEnable = param.depthWriteEnable,
    		.depthCompareOp = param.depthCompareOp,
    		.depthBoundsTestEnable = VK_FALSE,
    		.stencilTestEnable = VK_FALSE,
    		.minDepthBounds = 0.0f,
    		.maxDepthBounds = 0.0f,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttacment[MAX_ATTACHMENT_COUNT];

    	VkPipelineColorBlendStateCreateInfo colorBlendState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    		.logicOpEnable = VK_FALSE,
    		.logicOp = VK_LOGIC_OP_NO_OP,
    		.attachmentCount = 0,
    		.pAttachments = colorBlendAttacment,
    		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
		};

		if (!noFragmentShader)
		{
			colorBlendState.attachmentCount = param.colorTargetCount;
			for (uint32_t i = 0; i < param.colorTargetCount; ++ i)
			{
				auto& a = colorBlendAttacment[i];
    			a.blendEnable = param.blendEnable;
    			a.srcColorBlendFactor = param.srcColorBlendFactor;
    			a.dstColorBlendFactor = param.dstColorBlendFactor;
    			a.colorBlendOp = param.colorBlendOp;
    			a.srcAlphaBlendFactor = param.srcAlphaBlendFactor;
    			a.dstAlphaBlendFactor = param.dstAlphaBlendFactor;
    			a.alphaBlendOp = param.alphaBlendOp;
    			a.colorWriteMask = param.colorWriteMask;
			}
		}

		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    	VkPipelineDynamicStateCreateInfo dynamicState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    		.dynamicStateCount = 2,
    		.pDynamicStates = dynamicStates,
		};

		VkGraphicsPipelineCreateInfo CI
		{
    		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    		.stageCount = (uint32_t) stageInfos.size(),
    		.pStages = stageInfos.data(),
    		.pVertexInputState = &vertexInputInfo,
    		.pInputAssemblyState = &inputAssemblyState,
    		.pTessellationState = &tessellationState,
    		.pViewportState = &viewportState,
    		.pRasterizationState = &rasterizationState,
    		.pMultisampleState = &multisampleState,
    		.pDepthStencilState = &depthStencilState,
    		.pColorBlendState = &colorBlendState,
    		.pDynamicState = &dynamicState,
    		.layout = param.layout.GetHandle(),
    		.renderPass = param.renderPass.GetHandle(),
    		.subpass = param.subpassIdx,
    		.basePipelineHandle = VK_NULL_HANDLE,
		};
		bool success = false;
		VK_REPORT(vkCreateGraphicsPipelines(m_Ctx.Device, param.cache, 1, &CI, nullptr, &m_Handle), success);

		if (success)
		{
			if (param.dedicatedName)
				SetDedicatedDebugName(param.dedicatedName);
			else if (param.scopeName)
				SetDefaultDebugName(param.scopeName, nullptr);
			else
			 	COUST_CORE_WARN("Graphics pipeline created without debug name");
		}
		else  
			m_Handle = VK_NULL_HANDLE;
	}

	GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
		: Base(std::forward<Base>(other)),
		  Hashable(std::forward<Hashable>(other)),
		  m_Layout(other.m_Layout),
		  m_RenderPass(other.m_RenderPass),
		  m_SubpassIdx(other.m_SubpassIdx)
	{}

	GraphicsPipeline::~GraphicsPipeline()
	{
		if (m_Handle != VK_NULL_HANDLE)
			vkDestroyPipeline(m_Ctx.Device, m_Handle, nullptr);
	}

	const PipelineLayout& GraphicsPipeline::GetLayout() const noexcept { return m_Layout; }

	const RenderPass& GraphicsPipeline::GetRenderPass() const noexcept { return m_RenderPass; }

	uint32_t GraphicsPipeline::GetSubpassIndex() const noexcept { return m_SubpassIdx; }


	size_t PipelineLayout::ConstructParam::GetHash() const 
	{
		size_t h = 0;
        for (const auto& s : shaderModules)
		{
			Hash::Combine(h, s);
		}
		return h;
	}

	size_t GraphicsPipeline::ConstructParam::GetHash() const 
	{
		size_t h = 0;
		Hash::Combine(h, specializationConstantInfo);
		Hash::Combine(h, topology);
		Hash::Combine(h, polygonMode);
		Hash::Combine(h, cullMode);
		Hash::Combine(h, frontFace);
		Hash::Combine(h, depthBiasEnable);
		Hash::Combine(h, depthBiasConstantFactor);
		Hash::Combine(h, depthBiasClamp);
		Hash::Combine(h, depthBiasSlopeFactor);
		Hash::Combine(h, rasterizationSamples);
		Hash::Combine(h, depthWriteEnable);
		Hash::Combine(h, depthCompareOp);
		Hash::Combine(h, colorTargetCount);
		Hash::Combine(h, blendEnable);
		Hash::Combine(h, srcColorBlendFactor);
		Hash::Combine(h, dstColorBlendFactor);
		Hash::Combine(h, colorBlendOp);
		Hash::Combine(h, srcAlphaBlendFactor);
		Hash::Combine(h, dstAlphaBlendFactor);
		Hash::Combine(h, alphaBlendOp);
		Hash::Combine(h, colorWriteMask);
		Hash::Combine(h, layout);
		Hash::Combine(h, layout);
		Hash::Combine(h, subpassIdx);
		return h;
	}
}
