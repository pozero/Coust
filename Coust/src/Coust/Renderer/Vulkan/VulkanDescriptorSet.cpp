#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanDescriptorSet.h"

namespace Coust
{
	namespace VK
	{
		void DesriptorSetManager::Cleanup()
		{
			vkDeviceWaitIdle(g_Device);
			for (VkDescriptorSetLayout l : m_CreatedDescriptorSetLayouts)
			{
				vkDestroyDescriptorSetLayout(g_Device, l, nullptr);
			}

			for (VkDescriptorPool p : m_CreatedDescriptorPools)
			{
				vkDestroyDescriptorPool(g_Device, p, nullptr);
			}
		}

		inline VkDescriptorType GetBufferType(DesriptorSetManager::BufferType type)
		{
			switch (type)
			{
				case DesriptorSetManager::BufferType::UNIFORM:
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				case DesriptorSetManager::BufferType::STORAGE:
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				default:
					return VK_DESCRIPTOR_TYPE_MAX_ENUM;
			}
		}

		bool DesriptorSetManager::CreateDescriptorSetLayout(const Param& param, VkDescriptorSetLayout* out_DescirptorSetLayout)
		{
			uint32_t bind = 0;
			std::size_t bindingCount = param.buffers.size() + param.textures.size() + param.textureArrays.size();
			std::vector<VkDescriptorSetLayoutBinding> bindings(bindingCount);
			std::vector<VkDescriptorBindingFlags> bindingFlags(bindingCount);

			for (const auto& b : param.buffers)
			{
				bindings[bind].binding				= bind;
				bindings[bind].descriptorType		= GetBufferType(b.type);
				bindings[bind].descriptorCount		= 1;
				bindings[bind].stageFlags			= b.shaderStages;
				bindingFlags[bind] = 0u;
				bind++;
			}

			for (const auto& t : param.textures)
			{
				bindings[bind].binding				= bind;
				bindings[bind].descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				bindings[bind].descriptorCount		= 1;
				bindings[bind].stageFlags			= t.shaderStages;
				bindingFlags[bind] = 0u;
				bind++;
			}

			for (const auto& ta : param.textureArrays)
			{
				bindings[bind].binding				= bind;
				bindings[bind].descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				bindings[bind].descriptorCount		= (uint32_t) ta.textures.size();
				bindings[bind].stageFlags			= ta.shaderStages;
				bindingFlags[bind] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
				bind++;
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagInfo
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = (uint32_t) bindingCount,
				.pBindingFlags = bindingFlags.data(),
			};

			VkDescriptorSetLayoutCreateInfo layoutInfo
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = &bindingFlagInfo,
				.flags = 0u,
				.bindingCount = (uint32_t) bindingCount,
				.pBindings = bindings.data(),
			};
			VK_CHECK(vkCreateDescriptorSetLayout(g_Device, &layoutInfo, nullptr, out_DescirptorSetLayout));

			m_CreatedDescriptorSetLayouts.push_back(*out_DescirptorSetLayout);
			return true;
		}

		bool DesriptorSetManager::CreateDescriptorPool(const Param& param, uint32_t setCount, VkDescriptorPool* out_DescriptorPool)
		{
			uint32_t uniformBufferCount = 0;
			uint32_t storageBufferCount = 0;
			for (const auto& b : param.buffers)
			{
				if (b.type == BufferType::UNIFORM)
					++uniformBufferCount;
				if (b.type == BufferType::STORAGE)
					++storageBufferCount;
			}

			uint32_t sampledImageCount = (uint32_t) param.textures.size();
			for (const auto& ta : param.textureArrays)
			{
				sampledImageCount += (uint32_t) ta.textures.size();
			}

			std::vector<VkDescriptorPoolSize> poolSizes{};
			if (uniformBufferCount > 0)
			{
				VkDescriptorPoolSize size
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = setCount * uniformBufferCount,
				};
				poolSizes.push_back(size);
			}
			if (storageBufferCount > 0)
			{
				VkDescriptorPoolSize size
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = setCount * storageBufferCount,
				};
				poolSizes.push_back(size);
			}
			if (sampledImageCount > 0)
			{
				VkDescriptorPoolSize size
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = setCount * sampledImageCount,
				};
				poolSizes.push_back(size);
			}

			VkDescriptorPoolCreateInfo descriptorPoolInfo
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = (uint32_t) setCount,
				.poolSizeCount = (uint32_t) poolSizes.size(),
				.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data(),
			};
			VK_CHECK(vkCreateDescriptorPool(g_Device, &descriptorPoolInfo, nullptr, out_DescriptorPool));

			m_CreatedDescriptorPools.push_back(*out_DescriptorPool);
			return true;
		}

		bool DesriptorSetManager::CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* out_DescriptorSet)
		{
			VkDescriptorSetAllocateInfo info
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = pool,
				.descriptorSetCount = 1,
				.pSetLayouts = &layout,
			};
			VK_CHECK(vkAllocateDescriptorSets(g_Device, &info, out_DescriptorSet));

			return true;
		}

		void DesriptorSetManager::UpdateDescriptorSet(const Param& param, VkDescriptorSet set)
		{
			uint32_t binding = 0;
			std::vector<VkWriteDescriptorSet> writes;

			std::vector<VkDescriptorBufferInfo> bufferInfos(param.buffers.size());
			for (std::size_t i = 0; i < param.buffers.size(); ++i)
			{
				const BufferAttachment& b = param.buffers[i];
				bufferInfos[i].buffer = b.bufferAlloc.buffer;
				bufferInfos[i].offset = b.offset;
				bufferInfos[i].range  = (b.size > 0) ? b.size : VK_WHOLE_SIZE;

				VkWriteDescriptorSet w
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = set,
					.dstBinding = binding++,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = GetBufferType(b.type),
					.pBufferInfo = &bufferInfos[i],
				};
				writes.push_back(w);
			}

			std::vector<VkDescriptorImageInfo> imageInfos(param.textures.size());
			for (std::size_t i = 0; i < param.textures.size(); ++i)
			{
				const TextureAttachment& t = param.textures[i];
				imageInfos[i].sampler = t.texture.sampler;
				imageInfos[i].imageView = t.texture.imageView;
				imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet w
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = set,
					.dstBinding = binding++,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfos[i],
				};
				writes.push_back(w);
			}

			uint32_t imageArrayOffset = 0;
			std::vector<uint32_t> imageArrayOffsets(param.textureArrays.size());
			std::vector<VkDescriptorImageInfo> imageArrayInfos{};
			for (std::size_t i = 0; i < param.textureArrays.size(); ++i)
			{
				imageArrayOffsets[i] = imageArrayOffset;

				for (std::size_t j = 0; j < param.textureArrays[i].textures.size(); ++j)
				{
					const Texture& t = param.textureArrays[i].textures[j];
					VkDescriptorImageInfo info
					{
						.sampler = t.sampler,
						.imageView = t.imageView,
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					};
					imageArrayInfos.push_back(info);
				}

				imageArrayOffset += (uint32_t) param.textureArrays[i].textures.size();

			}
			for (std::size_t i = 0; i < param.textureArrays.size(); ++i)
			{
				VkWriteDescriptorSet w
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = set,
					.dstBinding = binding++,
					.dstArrayElement = 0,
					.descriptorCount = (uint32_t)param.textureArrays[i].textures.size(),
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageArrayInfos[imageArrayOffsets[i]],
				};
				writes.push_back(w);
			}

			vkUpdateDescriptorSets(g_Device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
		}
	}
}
