#pragma once

#include "volk/volk.h"

namespace Graphics
{

	// This should someday be a full class
	struct Frame
	{
		VkSemaphore renderSemaphore{};
		VkSemaphore presentSemaphore{};
		VkFence     renderFence{};

		VkCommandPool   cmdPool{};
		VkCommandBuffer cmdBuffer{};
	};

}