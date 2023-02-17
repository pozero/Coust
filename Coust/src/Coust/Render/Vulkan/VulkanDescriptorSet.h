#pragma once

#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	class DesriptorSetManager
	{
	public:
		enum class BufferType
		{
			UNIFORM,
			STORAGE,
		};

		struct BufferAttachment
		{
			BufferType type;
			VkShaderStageFlags shaderStages;

			uint32_t offset, size;
			BufferAlloc bufferAlloc;
		};

		struct TextureAttachment
		{
			// Descriptor type is hardcoded `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`
			VkShaderStageFlags shaderStages;

			Texture texture;
		};

		struct TextureArrayAttachment
		{
			// Descriptor type is hardcoded `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`
			VkShaderStageFlags shaderStages;

			std::vector<Texture> textures;
		};

		// binding number is incremented from first attachment
		struct Param
		{
			std::vector<BufferAttachment> buffers;
			std::vector<TextureAttachment> textures;
			std::vector<TextureArrayAttachment> textureArrays;
		};

	public:
		void Cleanup(const Context &ctx);

		bool CreateDescriptorSetLayout(const Context &ctx, const Param& param, VkDescriptorSetLayout* out_DescirptorSetLayout);

		bool CreateDescriptorPool(const Context &ctx, const Param& param, uint32_t setCount, VkDescriptorPool* out_DescriptorPool);

		bool CreateDescriptorSet(const Context &ctx, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* out_DescriptorSet);

		void UpdateDescriptorSet(const Context &ctx, const Param& param, VkDescriptorSet set);

	private:
		std::vector<VkDescriptorSetLayout> m_CreatedDescriptorSetLayouts{};
		std::vector<VkDescriptorPool> m_CreatedDescriptorPools{};
	};
}