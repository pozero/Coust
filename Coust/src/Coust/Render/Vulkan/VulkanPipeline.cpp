#include "pch.h"

#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK
{
	PipelineLayout::PipelineLayout(const ConstructParam& param) noexcept
		: Base(*param.ctx, VK_NULL_HANDLE),
		  Hashable(param.GetHash()),
		  m_ShaderModules(param.shaderModules),
		  m_ShaderResources(param.shaderResources), 
		  m_SetToResourceIdxLookup(param.setToResourceIdxLookup)
	{
		DEF_STLALLOC(ShaderResource, *param.ctx->StkArena, shaderResAllocator);
		DEF_STLALLOC(VkDescriptorSetLayout, *param.ctx->StkArena, descSetLayoutAllocator);
		DEF_STLALLOC(VkPushConstantRange, *param.ctx->StkArena, pushConstRangeAllocator);

		for (auto& pair : m_SetToResourceIdxLookup)
		{
			uint32_t set = pair.first;
			std::vector<ShaderResource, decltype(shaderResAllocator)> resources{ shaderResAllocator };
			resources.reserve(sizeof(pair.second));
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
            	.ctx = *param.ctx,
            	.set = set,
				.shaderModulesCount = param.shaderModules.size(),
            	.shaderModules = param.shaderModules.data(),
				.shaderResourcesCount = resources.size(),
            	.shaderResources = resources.data(),
            	.dedicatedName = debugName.c_str(),
			};
			m_DescriptorLayouts.emplace_back(p);

			COUST_CORE_PANIC_IF(!DescriptorSetLayout::CheckValidation(m_DescriptorLayouts.back()), "Can't create descriptor set layout for shader resources: {}", debugName);
		}
		std::vector<VkDescriptorSetLayout, decltype(descSetLayoutAllocator)> setLayouts{ descSetLayoutAllocator };
		setLayouts.reserve(m_DescriptorLayouts.size());
		for (const auto& l : m_DescriptorLayouts)
		{
			setLayouts.push_back(l.GetHandle());
		}

    	std::vector<VkPushConstantRange, decltype(pushConstRangeAllocator)> pushConstantRanges{ pushConstRangeAllocator };
		pushConstantRanges.reserve(m_ShaderResources.size());
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
		VK_CHECK(vkCreatePipelineLayout(m_Ctx.Device, &CI, nullptr, &m_Handle), "Can't create pipeline layout {} {}", param.dedicatedName, param.scopeName);
#ifndef COUST_FULL_RELEASE
		if (param.dedicatedName)
			SetDedicatedDebugName(param.dedicatedName);
		else if (param.scopeName)
			SetDefaultDebugName(param.scopeName, nullptr);
#endif
	}

	PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
		: Base(std::forward<Base>(other)),
		  Hashable(std::forward<Hashable>(other)),
          m_ShaderModules(std::move(other.m_ShaderModules)),
		  m_DescriptorLayouts(std::move(other.m_DescriptorLayouts)),
		  m_ShaderResources(std::move(other.m_ShaderResources)),
		  m_SetToResourceIdxLookup(std::move(other.m_SetToResourceIdxLookup))
	{}

	PipelineLayout::~PipelineLayout() noexcept
	{
		if (m_Handle != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_Ctx.Device, m_Handle, nullptr);
	}

	const std::vector<ShaderModule*>& PipelineLayout::GetShaderModules() const noexcept { return m_ShaderModules; }

	const std::vector<DescriptorSetLayout>& PipelineLayout::GetDescriptorSetLayouts() const noexcept { return m_DescriptorLayouts; }

	const std::vector<ShaderResource>& PipelineLayout::GetShaderResources() const noexcept { return m_ShaderResources; }

    void SpecializationConstantInfo::Reset() noexcept
	{
		m_Entry.clear();
		m_Data.clear();
	}

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

	GraphicsPipeline::GraphicsPipeline(const ConstructParam& param) noexcept
		: Base(*param.ctx, VK_NULL_HANDLE),
		  Hashable(param.GetHash()),
		  m_Layout(*param.layout),
		  m_RenderPass(*param.renderPass),
		  m_SubpassIdx(param.subpassIdx)
	{
		DEF_STLALLOC(VkPipelineShaderStageCreateInfo, *param.ctx->StkArena, shaderStageCIAllocator);
		DEF_STLALLOC(VkVertexInputBindingDescription, *param.ctx->StkArena, vtxInBindDescAllocator);
		DEF_STLALLOC(VkVertexInputAttributeDescription, *param.ctx->StkArena, vtxInAtribDescAllocator);

		bool noFragmentShader = true;
		const auto& shaderModules = param.layout->GetShaderModules();
		std::vector<VkPipelineShaderStageCreateInfo, decltype(shaderStageCIAllocator)> stageInfos{ shaderStageCIAllocator };
		stageInfos.reserve(shaderModules.size());

		VkSpecializationInfo specializationConstantInfo = param.specializationConstantInfo ? 
			param.specializationConstantInfo->Get() :
			VkSpecializationInfo{};
		VkSpecializationInfo* pSpecializationInfo = param.specializationConstantInfo ? 
			&specializationConstantInfo :
			nullptr;
		for (const auto& s : shaderModules)
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

		uint32_t perInstanceInputMask = 0u;
		std::vector<VkVertexInputBindingDescription, decltype(vtxInBindDescAllocator)> vertexBindingDescriptions{ vtxInBindDescAllocator };
        vertexBindingDescriptions.reserve(sizeof(perInstanceInputMask));
		std::vector<VkVertexInputAttributeDescription, decltype(vtxInAtribDescAllocator)> vertexAttributeDescriptions{ vtxInAtribDescAllocator };
        vertexAttributeDescriptions.reserve(4 * sizeof(perInstanceInputMask));
		ShaderModule::CollectShaderInputs(shaderModules, perInstanceInputMask, vertexBindingDescriptions, vertexAttributeDescriptions, *param.ctx->StkArena);

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
    		.topology = param.rasterState.topology,
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
    		.polygonMode = param.rasterState.polygonMode,
    		.cullMode = param.rasterState.cullMode,
    		.frontFace = param.rasterState.frontFace,
    		.depthBiasEnable = param.rasterState.depthBiasEnable,
    		.depthBiasConstantFactor = param.rasterState.depthBiasConstantFactor,
    		.depthBiasClamp = param.rasterState.depthBiasClamp,
    		.depthBiasSlopeFactor = param.rasterState.depthBiasSlopeFactor,
    		.lineWidth = 1.0f,
		};

    	VkPipelineMultisampleStateCreateInfo multisampleState
		{
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    		.rasterizationSamples = param.rasterState.rasterizationSamples,
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
    		.depthWriteEnable = param.rasterState.depthWriteEnable,
    		.depthCompareOp = param.rasterState.depthCompareOp,
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
			colorBlendState.attachmentCount = param.rasterState.colorTargetCount;
			for (uint32_t i = 0; i < param.rasterState.colorTargetCount; ++ i)
			{
				auto& a = colorBlendAttacment[i];
    			a.blendEnable = param.rasterState.blendEnable;
    			a.srcColorBlendFactor = param.rasterState.srcColorBlendFactor;
    			a.dstColorBlendFactor = param.rasterState.dstColorBlendFactor;
    			a.colorBlendOp = param.rasterState.colorBlendOp;
    			a.srcAlphaBlendFactor = param.rasterState.srcAlphaBlendFactor;
    			a.dstAlphaBlendFactor = param.rasterState.dstAlphaBlendFactor;
    			a.alphaBlendOp = param.rasterState.alphaBlendOp;
    			a.colorWriteMask = param.rasterState.colorWriteMask;
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
    		.layout = param.layout->GetHandle(),
    		.renderPass = param.renderPass->GetHandle(),
    		.subpass = param.subpassIdx,
    		.basePipelineHandle = VK_NULL_HANDLE,
		};
		VK_CHECK(vkCreateGraphicsPipelines(m_Ctx.Device, param.cache, 1, &CI, nullptr, &m_Handle), "Can't create graphics pipeline {} {}", param.dedicatedName, param.scopeName);

#ifndef COUST_FULL_RELEASE
		if (param.dedicatedName)
			SetDedicatedDebugName(param.dedicatedName);
		else if (param.scopeName)
			SetDefaultDebugName(param.scopeName, nullptr);
#endif
	}

	GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
		: Base(std::forward<Base>(other)),
		  Hashable(std::forward<Hashable>(other)),
		  m_Layout(other.m_Layout),
		  m_RenderPass(other.m_RenderPass),
		  m_SubpassIdx(other.m_SubpassIdx)
	{}

	GraphicsPipeline::~GraphicsPipeline() noexcept
	{
		if (m_Handle != VK_NULL_HANDLE)
			vkDestroyPipeline(m_Ctx.Device, m_Handle, nullptr);
	}

	const PipelineLayout& GraphicsPipeline::GetLayout() const noexcept { return m_Layout; }

	const RenderPass& GraphicsPipeline::GetRenderPass() const noexcept { return m_RenderPass; }

	uint32_t GraphicsPipeline::GetSubpassIndex() const noexcept { return m_SubpassIdx; }

	static constexpr uint32_t TIME_BEFORE_RELEASE = CommandBufferCache::COMMAND_BUFFER_COUNT;

	GraphicsPipelineCache::GraphicsPipelineCache(const Context& ctx) noexcept
        : m_PipelineLayoutRequirement{ &ctx },
          m_GraphicsPipelineRequirement{ &ctx },
          m_Ctx(ctx),
          m_Timer(TIME_BEFORE_RELEASE),
          m_PipelineLayoutHitCounter("GraphicsPipelineCache Pipeline Layout"),
          m_DescriptorSetHitCounter("GraphicsPipelineCache Descriptor Set"),
          m_GraphicsPipelineHitCounter("GraphicsPipelineCache Pipeline")
	{
	}

	void GraphicsPipelineCache::Reset() noexcept
	{
        m_CachedShaderModules.clear();
        m_CachedPipelineLayouts.clear();
        m_CachedDescriptorSets.clear();
        m_DescriptorSetAllocators.clear();
        m_CachedGraphicsPipelines.clear();
        m_SpecializationConstantCurrent.Reset();
		m_CurrentBoundShaderModules.clear();
        m_PipelineLayoutCurrent = nullptr;
        m_GraphicsPipelineCurrent = nullptr;
        m_DescriptorSetsCurrent.clear();
        m_PipelineLayoutRequirement = {};
        m_GraphicsPipelineRequirement = {};
        m_DescriptorSetRequirement.clear();
	}

	void GraphicsPipelineCache::GC(const CommandBuffer& buf) noexcept
	{
		m_Timer.Tick();
        m_SpecializationConstantCurrent.Reset();
		m_CurrentBoundShaderModules.clear();
        m_PipelineLayoutCurrent = nullptr;
        m_GraphicsPipelineCurrent = nullptr;
        m_DescriptorSetsCurrent.clear();
		m_DynamicOffsets.clear();

		for (auto iter = m_CachedDescriptorSets.begin(); iter != m_CachedDescriptorSets.end();)
		{
			auto& [set, lastAccessedTime] = iter->second;
			if (m_Timer.ShouldEvict(lastAccessedTime))
				iter = m_CachedDescriptorSets.erase(iter);
			else
			 	++ iter;
		}

		for (auto iter = m_CachedGraphicsPipelines.begin(); iter != m_CachedGraphicsPipelines.end();)
		{
			auto& [pipeline, lastAccessedTime] = iter->second;
			if (m_Timer.ShouldEvict(lastAccessedTime))
				iter = m_CachedGraphicsPipelines.erase(iter);
			else
			 	++ iter;
		}

		for (auto iter = m_CachedPipelineLayouts.begin(); iter != m_CachedPipelineLayouts.end();)
		{
			auto& [layout, lastAccessedTime] = iter->second;
			if (m_Timer.ShouldEvict(lastAccessedTime))
			{
				m_DescriptorSetAllocators.erase(&layout);
				iter = m_CachedPipelineLayouts.erase(iter);
			}
			else  
				++ iter;
		}
	}

	void GraphicsPipelineCache::UnBindDescriptorSet() noexcept
	{
		m_DescriptorSetRequirement.clear();
	}

	void GraphicsPipelineCache::UnBindPipeline() noexcept
	{
        m_PipelineLayoutRequirement = {};
        m_GraphicsPipelineRequirement = {};
	}

	SpecializationConstantInfo& GraphicsPipelineCache::BindSpecializationConstant() noexcept { return m_SpecializationConstantCurrent; }

	bool GraphicsPipelineCache::BindShader(const ShaderModule::ConstructParm& param) noexcept
	{
		size_t sourceHash = param.GetHash();
		auto iter = std::find_if(m_CachedShaderModules.begin(), m_CachedShaderModules.end(), 
			[sourceHash](decltype(*m_CachedShaderModules.cbegin()) sm) -> bool
			{
				return sm.GetHash() == sourceHash;
			});
		if (iter != m_CachedShaderModules.end())
		{
			m_CurrentBoundShaderModules.insert((uint32_t) std::distance(m_CachedShaderModules.begin(), iter));
			return true;
		}

		ShaderModule sm{ param };
		if (!ShaderModule::CheckValidation(sm))
		{
			COUST_CORE_ERROR("Failed to bind shader module (creation failed)");
			return false;
		}
		m_CachedShaderModules.push_back(std::move(sm));
		m_CurrentBoundShaderModules.insert((uint32_t) m_CachedShaderModules.size() - 1);
		return true;
	}

	void GraphicsPipelineCache::BindShaderFinished() noexcept
	{
		bool pipelineLayoutShaderUpdated = false;
		for (uint32_t i : m_CurrentBoundShaderModules)
		{
			size_t h = m_CachedShaderModules[i].GetHash();
			if (auto iter = std::find_if(m_PipelineLayoutRequirement.shaderModules.begin(), m_PipelineLayoutRequirement.shaderModules.end(),
				[h](const ShaderModule* s) -> bool
				{
					return s->GetHash() == h;
				}); iter == m_PipelineLayoutRequirement.shaderModules.end())
			{
				m_PipelineLayoutRequirement.shaderModules.push_back(&m_CachedShaderModules[i]);
				pipelineLayoutShaderUpdated = true;
			}
		}
		if (pipelineLayoutShaderUpdated)
		{
			m_PipelineLayoutRequirement.shaderResources.clear();
			m_PipelineLayoutRequirement.setToResourceIdxLookup.clear();
			ShaderModule::CollectShaderResources(m_PipelineLayoutRequirement.shaderModules, m_PipelineLayoutRequirement.shaderResources, m_PipelineLayoutRequirement.setToResourceIdxLookup);
		}

		for (auto& res : m_PipelineLayoutRequirement.shaderResources)
		{
			res.UpdateMode = ShaderResourceUpdateMode::Static;
		}
	}

	void GraphicsPipelineCache::SetAsDynamic(std::string_view name) noexcept
	{
		for (auto& res : m_PipelineLayoutRequirement.shaderResources)
		{
			if (res.Name == name)
				res.UpdateMode = ShaderResourceUpdateMode::Dynamic;
		}
	}

	bool GraphicsPipelineCache::BindPipelineLayout() noexcept
	{
		size_t requirementHash = m_PipelineLayoutRequirement.GetHash();
		if (auto iter = m_CachedPipelineLayouts.find(requirementHash); iter != m_CachedPipelineLayouts.end())
		{
			auto& [layout, lastAccessedTime] = iter->second;
			m_PipelineLayoutHitCounter.Hit();
			lastAccessedTime = m_Timer.CurrentCount();
			m_PipelineLayoutCurrent = (PipelineLayout*) &layout;
			FillDescriptorSetRequirements();
			return true;
		}
		
		m_PipelineLayoutHitCounter.Miss();

		PipelineLayout pl{ m_PipelineLayoutRequirement };
		if (!PipelineLayout::CheckValidation(pl))
		{
			COUST_CORE_ERROR("Failed to bind pipeline layout (creation failed)");
			return false;
		}
		auto iter = m_CachedPipelineLayouts.emplace(requirementHash, decltype(m_CachedPipelineLayouts.begin()->second){ std::move(pl), m_Timer.CurrentCount() });
		m_PipelineLayoutCurrent = (PipelineLayout*) &iter.first->second.first;
		CreateDescriptorAllocator();
		FillDescriptorSetRequirements();
		return true;
	}

	void GraphicsPipelineCache::BindRasterState(const GraphicsPipeline::RasterState& state) noexcept
	{
		m_GraphicsPipelineRequirement.rasterState = state;
	}

	void GraphicsPipelineCache::BindRenderPass(const RenderPass* renderPass, uint32_t subpassIdx) noexcept
	{
		m_GraphicsPipelineRequirement.renderPass = renderPass;
		m_GraphicsPipelineRequirement.subpassIdx = subpassIdx;
	}

	void GraphicsPipelineCache::BindBuffer(std::string_view name, const Buffer& buffer, uint64_t offset, uint64_t size, uint32_t arrayIdx) noexcept
	{
		for (const auto& res : m_PipelineLayoutRequirement.shaderResources)
		{
			if (res.Name == name)
			{
				uint64_t offsetToWrite = offset;
				// if we use dynamic offset, adjust the offset to 0, and store the dynamic offset
				if (res.UpdateMode == ShaderResourceUpdateMode::Dynamic)
				{
					offsetToWrite = 0;
					m_DynamicOffsets[res.Set] = (uint32_t) offset;
				}
				const uint32_t setIdx = res.Set;
				const uint32_t bindIdx = res.Binding;
				auto param = m_DescriptorSetRequirement[setIdx];
				auto arrInfo = param.bufferInfos[bindIdx];
				auto bufInfo = arrInfo.elements[arrayIdx];
				// We'll just overwrite the buffer info if there're already binding exists
				bufInfo.buffer = buffer.GetHandle();
				bufInfo.offset = offsetToWrite;
				bufInfo.range = size;
				bufInfo.dstArrayIdx = arrayIdx;
				return;
			}
			COUST_CORE_WARN("Can't find shader buffer resource of name {}", name);
		}
	}

	void GraphicsPipelineCache::BindImage(std::string_view name, VkSampler sampler, const Image& image, uint32_t arrayIdx) noexcept
	{
		for (const auto& res : m_PipelineLayoutRequirement.shaderResources)
		{
			if (res.Name == name)
			{
				const uint32_t setIdx = res.Set;
				const uint32_t bindIdx = res.Binding;
				auto param = m_DescriptorSetRequirement[setIdx];
				auto arrInfo = param.imageInfos[bindIdx];
				auto imageInfo = arrInfo.elements[arrayIdx];
				// We'll just overwrite the image info if there're already binding exists
				imageInfo.sampler = sampler;
				imageInfo.imageView = image.GetPrimaryView()->GetHandle();
				imageInfo.imageLayout = image.GetPrimaryLayout();
				imageInfo.dstArrayIdx = arrayIdx;
				return;
			}
		}
		COUST_CORE_WARN("Can't find shader image resource of name {}", name);
	}

	void GraphicsPipelineCache::BindInputAttachment(std::string_view name, const Image& attachment) noexcept
	{
		for (const auto& res : m_PipelineLayoutRequirement.shaderResources)
		{
			if (res.Name == name)
			{
				const uint32_t setIdx = res.Set;
				const uint32_t bindIdx = res.Binding;
				auto param = m_DescriptorSetRequirement[setIdx];
				auto arrInfo = param.imageInfos[bindIdx];
				// We'll just overwrite the input attachment info if there're already binding exists
				auto& imageInfo = arrInfo.elements[0];
				imageInfo.imageView = attachment.GetPrimaryView()->GetHandle();
				imageInfo.imageLayout = attachment.GetPrimaryLayout();
				return;
			}
		}
		COUST_CORE_WARN("Can't find shader input attachment resource of name {}", name);
	}

    void GraphicsPipelineCache::SetInputRatePerInstance(uint32_t location) noexcept
	{
		m_GraphicsPipelineRequirement.perInstanceInputMask |= (1u << location);
	}

	bool GraphicsPipelineCache::BindDescriptorSet(VkCommandBuffer cmdBuf) noexcept
	{
		DEF_STLALLOC(VkDescriptorSet, *m_Ctx.StkArena, dscSetAllocator);
		DEF_STLALLOC(uint32_t, *m_Ctx.StkArena, u32Allocator);

		std::vector<VkDescriptorSet, decltype(dscSetAllocator)> setToBind{ dscSetAllocator };
		setToBind.reserve(m_DescriptorSetRequirement.size());
		std::vector<uint32_t, decltype(u32Allocator)> dynamicOffsets{ u32Allocator };
		dynamicOffsets.reserve(m_DescriptorSetRequirement.size());
		// The descriptor requriement has already been sorted by set index. The spec says:
		// Values are taken from pDynamicOffsets in an order such that all entries for set N come before set
		// N+1; within a set, entries are ordered by the binding numbers in the descriptor set layouts; and
		// within a binding array, elements are in order. dynamicOffsetCount must equal the total number of
		// dynamic descriptors in the sets being bound.
		for (auto& requirement : m_DescriptorSetRequirement)
		{
			size_t h = requirement.GetHash();
			// if it's already bound, then don't bother
			if (auto iter = std::find_if(m_DescriptorSetsCurrent.begin(), m_DescriptorSetsCurrent.end(),
				[h](const DescriptorSet* d) -> bool
				{
					return d->GetHash() == h;
				}); iter != m_DescriptorSetsCurrent.end())
				continue;
			
			// next, find if it exists in descriptor cache
			if (auto iter = m_CachedDescriptorSets.find(h); iter != m_CachedDescriptorSets.end())
			{
				auto& [set, lastAccessedTime] = iter->second;
				m_DescriptorSetHitCounter.Hit();
				set.ApplyWrite(true);
				lastAccessedTime = m_Timer.CurrentCount();
				m_DescriptorSetsCurrent.push_back(&set);
				setToBind.push_back(set.GetHandle());
				if (m_DynamicOffsets.find(requirement.setIndex) != m_DynamicOffsets.end())
				{
					dynamicOffsets.push_back(m_DynamicOffsets[requirement.setIndex]);
				}
				continue;
			}

			// create a new descriptor set

			m_DescriptorSetHitCounter.Miss();

			requirement.dedicatedName = requirement.allocator->GetLayout().GetDebugName().c_str();
			DescriptorSet set{ requirement };
			if (!DescriptorSet::CheckValidation(set))
			{
				COUST_CORE_ERROR("Failed to bind descriptor sets (creation failure)");
				return false;
			}
			set.ApplyWrite(true);
			{
				auto iter = m_CachedDescriptorSets.emplace(h, decltype(m_CachedDescriptorSets.begin()->second) { std::move(set), m_Timer.CurrentCount() });
				auto& [set, lastAccessedTime] = iter.first->second;
				m_DescriptorSetsCurrent.push_back(&set);
				setToBind.push_back(set.GetHandle());
			}
			if (m_DynamicOffsets.find(requirement.setIndex) != m_DynamicOffsets.end())
			{
				dynamicOffsets.push_back(m_DynamicOffsets[requirement.setIndex]);
			}
		}

		if (!setToBind.empty())
			vkCmdBindDescriptorSets(cmdBuf, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				m_PipelineLayoutCurrent->GetHandle(), 
				0, (uint32_t) setToBind.size(), setToBind.data(), 
				(uint32_t) dynamicOffsets.size(), dynamicOffsets.data());
		return true;
	}

	bool GraphicsPipelineCache::BindPipeline(VkCommandBuffer cmdBuf) noexcept
	{
		m_GraphicsPipelineRequirement.specializationConstantInfo = &m_SpecializationConstantCurrent;
		m_GraphicsPipelineRequirement.layout = m_PipelineLayoutCurrent;
		m_GraphicsPipelineRequirement.cache = m_Cache;

		size_t h = m_GraphicsPipelineRequirement.GetHash();
		// if already bound, then early return
		if (m_GraphicsPipelineCurrent)
		{
			if (m_GraphicsPipelineCurrent->GetHash() == h)
				return true;
		}

		// else find in graphics pipeline cache
		if (auto iter = m_CachedGraphicsPipelines.find(h); iter != m_CachedGraphicsPipelines.end())	
		{
			m_GraphicsPipelineHitCounter.Hit();
			auto& [pipeline, lastAccessedTime] = iter->second;
			lastAccessedTime = m_Timer.CurrentCount();
			m_GraphicsPipelineCurrent = &pipeline;
			vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle());
			return true;
		}

		// else create a new pipeline

		m_GraphicsPipelineHitCounter.Miss();

		GraphicsPipeline p { m_GraphicsPipelineRequirement };
		if (!GraphicsPipeline::CheckValidation(p))
		{
			COUST_CORE_ERROR("Failed to bind graphics pipeline (creation failure)");
			return false;
		}
		{
			auto iter = m_CachedGraphicsPipelines.emplace(h, decltype(m_CachedGraphicsPipelines.begin()->second) { std::move(p), m_Timer.CurrentCount() });
			auto& [pipeline, lastAccessedTime] = iter.first->second;
			m_GraphicsPipelineCurrent = &pipeline;
			vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle());
		}
		return true;
	}

    void GraphicsPipelineCache::CreateDescriptorAllocator() noexcept
	{
		auto iter = m_DescriptorSetAllocators.emplace(m_PipelineLayoutCurrent, std::vector<DescriptorSetAllocator>{});
		
		for (const auto& l : m_PipelineLayoutCurrent->GetDescriptorSetLayouts())
		{
			DescriptorSetAllocator::ConstructParam p
			{
				.ctx = m_Ctx,
				.layout = l,
			};
			DescriptorSetAllocator a{ p };
			iter.first->second.emplace_back(std::move(a));
		}
	}

    void GraphicsPipelineCache::FillDescriptorSetRequirements() noexcept
	{
		m_DescriptorSetRequirement.clear();
		const auto& v = m_DescriptorSetAllocators.at(m_PipelineLayoutCurrent);
		// To avoid too much searching when binding resources later, we fill in all possible descriptor sets.
		uint32_t biggestSetIdx = 0;
		for (const auto& alloc : v)
		{
			biggestSetIdx = std::max(alloc.GetLayout().GetSetIndex(), biggestSetIdx);
		}
		m_DescriptorSetRequirement.resize(biggestSetIdx + 1);
		for (const auto& alloc : v)
		{
			auto& r = m_DescriptorSetRequirement[alloc.GetLayout().GetSetIndex()];
			r.ctx = &m_Ctx;
			alloc.FillEmptyDescriptorSetConstructParam(r);
		}
	}

	size_t PipelineLayout::ConstructParam::GetHash() const noexcept
	{
		size_t h = 0;
        for (const auto& s : shaderModules)
		{
			Hash::Combine(h, s);
		}

		// Different update mode will result in different descriptor set layouts
		for (const auto& res : shaderResources)
		{
			Hash::Combine(h, res.UpdateMode);
		}
		return h;
	}

	size_t GraphicsPipeline::ConstructParam::GetHash() const noexcept
	{
		size_t h = 0;
		Hash::Combine(h, *specializationConstantInfo);
		Hash::Combine(h, rasterState.topology);
		Hash::Combine(h, rasterState.polygonMode);
		Hash::Combine(h, rasterState.cullMode);
		Hash::Combine(h, rasterState.frontFace);
		Hash::Combine(h, rasterState.depthBiasEnable);
		Hash::Combine(h, rasterState.depthBiasConstantFactor);
		Hash::Combine(h, rasterState.depthBiasClamp);
		Hash::Combine(h, rasterState.depthBiasSlopeFactor);
		Hash::Combine(h, rasterState.rasterizationSamples);
		Hash::Combine(h, rasterState.depthWriteEnable);
		Hash::Combine(h, rasterState.depthCompareOp);
		Hash::Combine(h, rasterState.colorTargetCount);
		Hash::Combine(h, rasterState.blendEnable);
		Hash::Combine(h, rasterState.srcColorBlendFactor);
		Hash::Combine(h, rasterState.dstColorBlendFactor);
		Hash::Combine(h, rasterState.colorBlendOp);
		Hash::Combine(h, rasterState.srcAlphaBlendFactor);
		Hash::Combine(h, rasterState.dstAlphaBlendFactor);
		Hash::Combine(h, rasterState.alphaBlendOp);
		Hash::Combine(h, rasterState.colorWriteMask);
		Hash::Combine(h, *layout);
		Hash::Combine(h, *renderPass);
		Hash::Combine(h, subpassIdx);
		return h;
	}
}
