#include "device.hpp"

#include "instance.hpp"

#include "volk/volk.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <iostream>

namespace Graphics
{

	Device::Device(const Instance& instance)
		: m_instance{ instance }
	{
		std::uint32_t gpuCount{};
		vkEnumeratePhysicalDevices(m_instance.vkInstance(), &gpuCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		vkEnumeratePhysicalDevices(m_instance.vkInstance(), &gpuCount, physicalDevices.data());

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

		VkPhysicalDeviceVulkan13Features vulkan13Features{ .sType{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES } };
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;

		constexpr float queuePriority{ 1.0f };

		VkDeviceQueueCreateInfo queueInfo{ .sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO } };
		queueInfo.queueFamilyIndex = m_graphicsQueueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;

		std::vector<const char*> extensions{};
		extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo deviceInfo{ .sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO } };
		deviceInfo.pNext = &vulkan13Features;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = extensions.size();
		deviceInfo.ppEnabledExtensionNames = extensions.data();

		if (vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device) != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: Vulkan failed to create a logical device.\n" };
		}

		volkLoadDevice(m_device);

		vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

		VmaVulkanFunctions vkFcns{ .vkGetInstanceProcAddr{ vkGetInstanceProcAddr }, .vkGetDeviceProcAddr{ vkGetDeviceProcAddr } };

		VmaAllocatorCreateInfo  allocInfo{};
		allocInfo.physicalDevice = m_physicalDevice;
		allocInfo.device = m_device;
		allocInfo.pVulkanFunctions = &vkFcns;
		allocInfo.instance = m_instance.vkInstance();
		allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		
		if (vmaCreateAllocator(&allocInfo, &m_allocator) != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: Vulkan Memory Allocator failed to create an allocator.\n" };
		}
	}

	Device::~Device()
	{
		vmaDestroyAllocator(m_allocator);
		vkDestroyDevice(m_device, nullptr);
	}

}