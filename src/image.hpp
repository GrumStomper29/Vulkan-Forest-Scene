#pragma once

#include "volk/volk.h"

#include "vma/vk_mem_alloc.h"

namespace Graphics
{

	struct Image
	{
		VkImage image{};
		VmaAllocation alloc{};
	};

}