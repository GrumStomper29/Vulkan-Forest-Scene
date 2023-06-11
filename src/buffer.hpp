#pragma once

#include "volk/volk.h"
#include "vma/vk_mem_alloc.h"

namespace Graphics
{

	struct Buffer
	{
		VkBuffer buffer{};
		VmaAllocation alloc{};
	};

}