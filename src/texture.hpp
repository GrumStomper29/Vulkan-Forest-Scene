#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "vma/vk_mem_alloc.h"

namespace Graphics
{

	Image loadSkybox(const char** paths, VkDevice device, VmaAllocator allocator,
		VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

	VkImageView createSkyboxView(VkDevice device, VkImage image);

	VkSampler createSkyboxSampler(VkDevice device);

}