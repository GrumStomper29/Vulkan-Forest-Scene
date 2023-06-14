#include "device.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <vector>

namespace Graphics
{

	VkDevice createDevice(VkPhysicalDevice physicalDevice, std::uint32_t graphicsQueueFamily, VkQueue& graphicsQueue)
	{
		VkPhysicalDeviceVulkan13Features vulkan13Features
		{
			.sType{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES },
			.dynamicRendering{ VK_TRUE },
		};

		constexpr float graphicsQueuePriority{ 1.0f };

		VkDeviceQueueCreateInfo graphicsQueueCI
		{
			.sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO },
			.queueFamilyIndex{ graphicsQueueFamily },
			.queueCount{ 1 },
			.pQueuePriorities{ &graphicsQueuePriority }
		};

		std::vector<const char*> extensions{};

		extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo deviceCI
		{
			.sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO },
			.pNext{ &vulkan13Features },
			.queueCreateInfoCount{ 1 },
			.pQueueCreateInfos{ &graphicsQueueCI },
			.enabledExtensionCount{ static_cast<std::uint32_t>(extensions.size()) },
			.ppEnabledExtensionNames{ extensions.data() },
		};

		VkDevice device{};
		vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device);

		volkLoadDevice(device);

		vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);

		return device;
	}

}