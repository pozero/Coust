#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cctype>

namespace Coust
{
	namespace VK
	{
		struct BufferAlloc
		{
			VkBuffer buffer;
			VmaAllocation alloc;
		};

		struct ImageAlloc
		{
			VkImage image;
			VmaAllocation alloc;
		};

		struct Texture
		{
			uint32_t width, height, depth;
			VkFormat format;

			ImageAlloc imageAlloc;
			VkImageView imageView;
			VkSampler sampler;
			VkImageLayout desiredLayout;
		};

		/* Helper Structs to Create Descriptor Sets */
		/* Helper Structs to Create Descriptor Sets */
	}
}