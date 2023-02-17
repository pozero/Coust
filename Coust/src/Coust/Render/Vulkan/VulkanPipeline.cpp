#include "pch.h"

#include "Coust/Render/Vulkan/VulkanPipeline.h"
#include "Coust/Render/Resources/Shader.h"

#include <stdint.h>

namespace Coust::Render::VK
{
	inline VkShaderStageFlagBits GetShaderStage(Shader::Type type)
	{
		switch (type)
		{
			case Shader::Type::VERTEX:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case Shader::Type::FRAGMENT:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case Shader::Type::TESSELLATION_CONTROL:
				return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case Shader::Type::TESSELLATION_EVALUATION:
				return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case Shader::Type::GEOMETRY:
				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case Shader::Type::COMPUTE:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			default:
				return VK_SHADER_STAGE_ALL_GRAPHICS;
		}
	}

	// TODO: Save cache data to files
	bool PipelineManager::Initialize(const Context &ctx)
	{
		VkPipelineCacheCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			.initialDataSize = 0,
		};

		std::vector<char> serilizedPipelineCache{};
		if (GlobalContext::Get().GetFileSystem().GetCache("vulkan_pipeline_cache", serilizedPipelineCache))
		{
			bool validated = true;
			const uint32_t* validationData = (const uint32_t*) serilizedPipelineCache.data();
			validated = validationData[0] == ctx.m_PhysicalDevProps.vendorID && 
						validationData[1] == ctx.m_PhysicalDevProps.deviceID &&
						validationData[2] == ctx.m_PhysicalDevProps.driverVersion;

			const uint8_t* validationDataByte = (const uint8_t*) (validationData + 3);
			validated = std::memcmp(validationDataByte, ctx.m_PhysicalDevProps.pipelineCacheUUID, VK_UUID_SIZE * sizeof(uint8_t)) == 0;

			if (validated)
			{
				info.initialDataSize = (uint32_t) (serilizedPipelineCache.size() - 3 * sizeof(uint32_t) - VK_UUID_SIZE * sizeof(uint8_t));
				info.pInitialData = validationDataByte + VK_UUID_SIZE;
			}
		}

		VK_CHECK(vkCreatePipelineCache(ctx.m_Device, &info, nullptr, &m_GraphicsCache));
		return true;
	}

	void PipelineManager::Cleanup(const Context &ctx)
	{
		if (size_t cacheSize = 0; vkGetPipelineCacheData(ctx.m_Device, m_GraphicsCache, &cacheSize, nullptr) == VK_SUCCESS && cacheSize > 0)
		{
			std::vector<char> pipelineCacheBytes{};
			pipelineCacheBytes.resize(3 * sizeof(uint32_t) + VK_UUID_SIZE * sizeof(uint8_t) + cacheSize);
			uint32_t* header = (uint32_t*) pipelineCacheBytes.data();
			header[0] = ctx.m_PhysicalDevProps.vendorID; 
			header[1] = ctx.m_PhysicalDevProps.deviceID;
			header[2] = ctx.m_PhysicalDevProps.driverVersion;
			std::memcpy(header + 3, ctx.m_PhysicalDevProps.pipelineCacheUUID, VK_UUID_SIZE * sizeof(uint8_t));
			if (vkGetPipelineCacheData(ctx.m_Device, m_GraphicsCache, &cacheSize, pipelineCacheBytes.data() + 3 * sizeof(uint32_t) + VK_UUID_SIZE * sizeof(uint8_t)) == VK_SUCCESS)
			{
				GlobalContext::Get().GetFileSystem().AddCache("vulkan_pipeline_cache", pipelineCacheBytes, true);
			}
		}

		vkDestroyPipelineCache(ctx.m_Device, m_GraphicsCache, nullptr);

		for (VkPipelineLayout l : m_UsedLayout)
		{
			vkDestroyPipelineLayout(ctx.m_Device, l, nullptr);
		}

		for (VkPipeline p : m_CreatedPipeline)
		{
			vkDestroyPipeline(ctx.m_Device, p, nullptr);
		}
	}

	bool PipelineManager::BuildGraphics(const Context &ctx, const Swapchain &swapchain, const PipelineManager::Param& param, VkPipeline* out_Pipeline)
	{
		if (param.shaderFiles.size() != param.macroes.size())
		{
			COUST_CORE_ERROR("{}: The number of macro arrays should be equal to the number of shader files", __FUNCTION__);
			return false;
		}

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
		for (std::size_t i = 0; i < param.shaderFiles.size(); ++i)
		{
			Shader shader{ param.shaderFiles[i], param.macroes[i] };

			VkShaderModule module;
			{
				VkShaderModuleCreateInfo info
				{
					.sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.codeSize   = shader.GetByteCode().size() * sizeof(uint32_t),
					.pCode      = shader.GetByteCode().data(),
				};
				VK_CHECK(vkCreateShaderModule(ctx.m_Device, &info, nullptr, &module));
			}

			VkPipelineShaderStageCreateInfo info
			{
				.sType      = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage      = GetShaderStage(shader.m_Type),
				.module     = module,
				.pName      = "main",
			};
			shaderStages.push_back(info);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState
		{
			.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount      = 0,
			.pVertexBindingDescriptions         = nullptr,
			.vertexAttributeDescriptionCount    = 0,
			.pVertexAttributeDescriptions       = nullptr,
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState
		{
			.sType                      = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology                   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable     = VK_FALSE,
		};

		// VkPipelineTessellationStateCreateInfo tessellationState
		// {
		//     .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		// };

		VkViewport viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float) swapchain.m_Extent.width,
			.height = (float) swapchain.m_Extent.width,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		VkRect2D scissor
		{
			.offset = { 0, 0 },
			.extent = swapchain.m_Extent,
		};
		VkPipelineViewportStateCreateInfo viewportState
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor,
		};

		VkPipelineRasterizationStateCreateInfo rasterizationState
		{
			.sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable           = VK_FALSE,
			.rasterizerDiscardEnable    = VK_FALSE,
			.polygonMode                = VK_POLYGON_MODE_FILL,
			.cullMode                   = VK_CULL_MODE_BACK_BIT,
			// default face winding for assimp is CCW
			.frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable            = VK_FALSE,
			.lineWidth                  = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampleState
		{
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples   = ctx.m_MSAASampleCount,
			.sampleShadingEnable    = VK_FALSE,
			.minSampleShading       = 1.0f,
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilState
		{
			.sType                      = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable            = VK_TRUE,
			.depthWriteEnable           = VK_TRUE,
			.depthCompareOp             = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable      = VK_FALSE,
			.stencilTestEnable          = VK_FALSE,
		};

		const VkPipelineColorBlendAttachmentState colorBlendAttachment
		{
			.blendEnable            = VK_TRUE,
			.srcColorBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp           = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor    = param.useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp           = VK_BLEND_OP_ADD,
			.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo colorBlendState
		{
			.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable      = VK_FALSE,
			.logicOp            = VK_LOGIC_OP_COPY,
			.attachmentCount    = 1,
			.pAttachments       = &colorBlendAttachment,
			.blendConstants     = {0.0f, 0.0f, 0.0f, 0.0f},
		};
		
		VkDynamicState dynamicStates[]
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		VkPipelineDynamicStateCreateInfo dynamicState
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = (uint32_t) ARRAYSIZE(dynamicStates),
			.pDynamicStates = dynamicStates,
		};

		VkGraphicsPipelineCreateInfo pipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = (uint32_t)shaderStages.size(),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pTessellationState = nullptr,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = param.useDepth ? &depthStencilState : nullptr,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = param.layout,
			.renderPass = param.renderpass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};
		VK_CHECK(vkCreateGraphicsPipelines(ctx.m_Device, m_GraphicsCache, 1, &pipelineInfo, nullptr, out_Pipeline));

		m_CreatedPipeline.push_back(*out_Pipeline);
		m_UsedLayout.insert(param.layout);

		for (const auto& i : shaderStages)
		{
			vkDestroyShaderModule(ctx.m_Device, i.module, nullptr);
		}

		return true;
	}
}