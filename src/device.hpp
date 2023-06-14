#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	VkDevice createDevice(VkPhysicalDevice physicalDevice, std::uint32_t graphicsQueueFamily, VkQueue& graphicsQueue);

}