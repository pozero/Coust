#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

namespace Coust
{
	namespace VK
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
			void Cleanup();

			bool CreateDescriptorSetLayout(const Param& param, VkDescriptorSetLayout* out_DescirptorSetLayout);

			bool CreateDescriptorPool(const Param& param, uint32_t setCount, VkDescriptorPool* out_DescriptorPool);

			bool CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* out_DescriptorSet);

			void UpdateDescriptorSet(const Param& param, VkDescriptorSet set);

		private:
			std::vector<VkDescriptorSetLayout> m_CreatedDescriptorSetLayouts{};
			std::vector<VkDescriptorPool> m_CreatedDescriptorPools{};
		};
	}
}