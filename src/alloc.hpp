#pragma once

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

namespace Graphics
{

	struct Buffer
	{
		VkBuffer buffer{};
		VmaAllocation alloc{};
	};

	struct Image
	{
		VkImage image{};
		VmaAllocation alloc{};
	};

	VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

}