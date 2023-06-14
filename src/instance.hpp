#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	VkInstance createInstance(bool enableValidation);

	VkPhysicalDevice getPhysicalDevice(VkInstance instance);

	std::uint32_t getGraphicsQueueFamily(VkPhysicalDevice physicalDevice);

}