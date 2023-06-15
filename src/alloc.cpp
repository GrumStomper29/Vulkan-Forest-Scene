#include "alloc.hpp"

#include "volk/volk.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_FUNCTIONS 1
#include "VMA/vk_mem_alloc.h"

namespace Graphics
{

	VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
	{
		VmaVulkanFunctions vkFcns
		{
			.vkGetInstanceProcAddr{ vkGetInstanceProcAddr },
			.vkGetDeviceProcAddr{ vkGetDeviceProcAddr },
		};

		VmaAllocatorCreateInfo allocatorCI
		{
			.physicalDevice{ physicalDevice },
			.device{ device },
			.pVulkanFunctions{ &vkFcns },
			.instance{ instance },
			.vulkanApiVersion{ VK_API_VERSION_1_3 },
		};

		VmaAllocator allocator{};
		vmaCreateAllocator(&allocatorCI, &allocator);

		return allocator;
	}

}