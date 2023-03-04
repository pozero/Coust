#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <vector>
#include <unordered_set>
#include <compare>

namespace Coust::Render::VK
{
	class RenderPass;

	class PipelineState;
	class PipelineLayout;
	
	struct VertexInputBinding
	{
   		uint32_t             binding;
   		uint32_t             stride;
   		VkVertexInputRate    inputRate;
		
		auto operator<=>(const VertexInputBinding&) const = default;
	};
	
	struct VertexInputAttribute 
	{
   		uint32_t    location;
   		uint32_t    binding;
   		VkFormat    format;
   		uint32_t    offset;
		
		auto operator<=>(const VertexInputAttribute&) const = default;
	};
	
	struct VertexInputState
	{
		std::vector<VertexInputBinding> 	VertexBindingDescriptions;
		std::vector<VertexInputAttribute> 	VertexAttributeDescriptions;
		
		auto operator<=>(const VertexInputState&) const = default;
	};
	
	struct InputAssemblyState 
	{
    	VkPrimitiveTopology 	Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkBool32 				PrimitiveRestartEnable = VK_FALSE;
		
		auto operator<=>(const InputAssemblyState&) const = default;
	};
	
	struct RasterizationState 
	{
    	VkBool32              DepthClampEnable = VK_FALSE;
   		VkBool32              RasterizerDiscardEnable = VK_FALSE;
   		VkPolygonMode         PolygonMode = VK_POLYGON_MODE_FILL;
   		VkCullModeFlags       CullMode = VK_CULL_MODE_BACK_BIT;
		// Note: We will use Assimp as our model library, whose default face winding is CCW
   		VkFrontFace           FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   		VkBool32              DepthBiasEnable = VK_FALSE;
   		float                 LineWidth = 1.0f;
		
		auto operator<=>(const RasterizationState&) const = default;
	};
	
	struct MultisampleState 
	{
		// Can be set by `Context.MSAASampleCount`
    	VkSampleCountFlagBits		RasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   		VkBool32                    SampleShadingEnable = VK_FALSE;
   		float                       MinSampleShading = 0.0f;
   		VkSampleMask				PSampleMask = 0;
   		VkBool32                    AlphaToCoverageEnable = VK_FALSE;
   		VkBool32                    AlphaToOneEnable = VK_FALSE;
		
		auto operator<=>(const MultisampleState&) const = default;
	};
	
	struct StencilOpState 
	{
   		VkStencilOp    failOp = VK_STENCIL_OP_REPLACE;
   		VkStencilOp    passOp = VK_STENCIL_OP_REPLACE;
		// Action when samples pass the stencil test but fail the depth test
   		VkStencilOp    depthFailOp = VK_STENCIL_OP_REPLACE;
   		VkCompareOp    compareOp = VK_COMPARE_OP_NEVER;
   		uint32_t       compareMask;
   		uint32_t       writeMask;
   		uint32_t       reference;
		
		auto operator<=>(const StencilOpState&) const = default;
	};
	
	struct DepthStencilState 
	{
		VkBool32            DepthTestEnable = VK_TRUE;
		VkBool32            DepthWriteEnable = VK_TRUE;
		// https://github.com/KhronosGroup/Vulkan-Samples/pull/25
		// TODO: Use Reversed depth-buffer get more even distribution of precision
		VkCompareOp         DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		VkBool32            DepthBoundsTestEnable = VK_FALSE;
		VkBool32            StencilTestEnable = VK_FALSE;
		StencilOpState 		Front;
		StencilOpState    	Back;
		// float               MinDepthBounds;
		// float               MaxDepthBounds;
		
		auto operator<=>(const DepthStencilState&) const = default;
	};
	
	struct ColorBlendAttachment 
	{
   		VkBool32                 blendEnable = VK_FALSE;
   		VkBlendFactor            srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   		VkBlendFactor            dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
   		VkBlendOp                colorBlendOp = VK_BLEND_OP_ADD;
   		VkBlendFactor            srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   		VkBlendFactor            dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   		VkBlendOp                alphaBlendOp = VK_BLEND_OP_ADD;
   		VkColorComponentFlags    colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		
		auto operator<=>(const ColorBlendAttachment&) const = default;
	};
	
	struct ColorBlendState
	{
   		VkBool32                            logicOpEnable = VK_FALSE;
   		VkLogicOp                           logicOp = VK_LOGIC_OP_COPY;
   		std::vector<ColorBlendAttachment>	pAttachments;
   		float                               blendConstants[4] { 0.0f, 0.0f, 0.0f, 0.0f };
		
		auto operator<=>(const ColorBlendState&) const = default;
	};
	
	/**
	 * @brief Basically a wrapper class of `VkGraphicsPipelineCreateInfo`
	 */
	class PipelineState
	{
	public:
		PipelineState() = default;
		~PipelineState() = default;

		void Reset();
		
		/* Setter */

		void SetVertexInputState(const VertexInputState& vertexInputState);
		
		void SetInputAssemblyState(const InputAssemblyState& inputAssembleState);
		
		void SetRasterizationState(const RasterizationState& rasterizationState);
		
		void SetMultisampleState(const MultisampleState& multisampleState);
		
		void SetDepthStencilState(const DepthStencilState& depthStencilState);
		
		void SetColorBlendState(const ColorBlendState& colorBlendState);

		void SetPipelineLayout(const PipelineLayout& pipelineLayout);
		
		void SetRenderPass(const RenderPass& renderPass);
		
		void SetSubpass(const uint32_t subpassIndex);
		
		/* Getter */

		const VertexInputState& GetVertexInputState() const { return m_VertexInputState; }
		
		const InputAssemblyState& GetInputAssemblyState() const { return m_InputAssembleState; }
		
		const RasterizationState& GetRasterizationState() const { return m_RasterizationState; }
		
		const MultisampleState& GetMultisampleState() const { return m_MultisampleState; }
		
		const DepthStencilState& GetDepthStencilState() const { return m_DepthStencilState; }
		
		const ColorBlendState& GetColorBlendState() const { return m_ColorBlendState; }
		
		const PipelineLayout* GetPipelineLayout() const { return m_PipelineLayout; }
		
		const RenderPass* GetRenderPass() const { return m_RenderPass; }
		
		const uint32_t GetSubpassIndex() const { return m_SubpassIndex; }

		/* Getter */
		
		/**
		 * @brief PipelienState gets dirty when its changes not being applied yet.
		 * @return 
		 */
		bool IsDirty() const { return m_Dirty; }

		/**
		 * @brief The changes have been applied, so flush its dirt.
		 */
		void Flush() { m_Dirty = false; }

	private:
		bool m_Dirty = false;
		
		VertexInputState m_VertexInputState{};
		
		InputAssemblyState m_InputAssembleState{};
		
		RasterizationState m_RasterizationState{};
		
		MultisampleState m_MultisampleState{};
		
		DepthStencilState m_DepthStencilState{};
		
		ColorBlendState m_ColorBlendState{};

		const PipelineLayout* m_PipelineLayout = nullptr;
		
		const RenderPass* m_RenderPass = nullptr;
		
		uint32_t m_SubpassIndex = 0;
	};
	
	class PipelineLayout : public Resource<VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT>
	{
	public:
		using Base = Resource<VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT>;
	};

}
