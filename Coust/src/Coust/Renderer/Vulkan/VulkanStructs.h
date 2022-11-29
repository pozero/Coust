#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

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
	}
}