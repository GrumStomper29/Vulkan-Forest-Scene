#include "device.hpp"

#include "instance.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Graphics
{

	Device::Device(const Instance& instance)
	{
		std::uint32_t gpuCount{};
		vkEnumeratePhysicalDevices(instance.getVkInstanceHandle(), &gpuCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		vkEnumeratePhysicalDevices(instance.getVkInstanceHandle(), &gpuCount, physicalDevices.data());

		m_physicalDevice = physicalDevices[0];

		std::uint32_t queueFamilyPropertyCount{};
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

		for (int i{ 0 }; i < queueFamilyProperties.size(); ++i)
		{
			if (queueFamilyProperties[i].queueFlags && VK_QUEUE_GRAPHICS_BIT)
			{
				m_graphicsQueueFamily = i;
				break;
			}
		}

		constexpr float queuePriority{ 1.0f };

		VkDeviceQueueCreateInfo queueInfo{ .sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO } };
		queueInfo.queueFamilyIndex = m_graphicsQueueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;

		std::vector<const char*> extensions{};
		extensions.push_back("VK_KHR_swapchain");

		VkDeviceCreateInfo deviceInfo{ .sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO } };
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = extensions.size();
		deviceInfo.ppEnabledExtensionNames = extensions.data();

		if (vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device) != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: Vulkan failed to create a logical device.\n" };
		}

		volkLoadDevice(m_device);
	}

	Device::~Device()
	{
		vkDestroyDevice(m_device, nullptr);
	}

}