#include "sync.hpp"

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	std::uint64_t secondsToNanoseconds(std::uint64_t ns)
	{
		return ns * 1'000'000'000;
	}

	VkFence createFence(VkDevice device, bool signaled)
	{
		VkFenceCreateInfo fenceCI
		{
			.sType{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO },
			.flags{ signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlags>(0) }
		};

		VkFence fence{};
		vkCreateFence(device, &fenceCI, nullptr, &fence);

		return fence;
	}

	VkSemaphore createSemaphore(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreCI
		{
			.sType{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO },
		};

		VkSemaphore semaphore{};
		vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore);

		return semaphore;
	}

}