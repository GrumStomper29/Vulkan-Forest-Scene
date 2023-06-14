#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{
	
	std::uint64_t secondsToNanoseconds(std::uint64_t ns);

	VkFence createFence(VkDevice device, bool signaled);

	VkSemaphore createSemaphore(VkDevice device);

}