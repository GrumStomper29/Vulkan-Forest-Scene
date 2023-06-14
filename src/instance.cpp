#include "instance.hpp"

#include "volk/volk.h"
#include "GLFW/glfw3.h"

#include <cstdint>
#include <exception>
#include <vector>

namespace Graphics
{

	VkInstance createInstance(bool enableValidation)
	{
		VkApplicationInfo appInfo
		{
			.sType{ VK_STRUCTURE_TYPE_APPLICATION_INFO },
			.apiVersion{ VK_API_VERSION_1_3 },
		};

		std::vector<const char*> layers{};
		if (enableValidation)
		{
			// Do layers have name constants like extensions?
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		std::vector<const char*> extensions{};

		std::uint32_t glfwExtensionCount{};
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		for (std::uint32_t i{ 0 }; i < glfwExtensionCount; ++i)
		{
			extensions.push_back(*(glfwExtensions + i));
		}

		VkInstanceCreateInfo instanceCI
		{
			.sType{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
			.pApplicationInfo{ &appInfo },
			.enabledLayerCount{ static_cast<std::uint32_t>(layers.size()) },
			.ppEnabledLayerNames{ layers.data() },
			.enabledExtensionCount{ static_cast<std::uint32_t>(extensions.size()) },
			.ppEnabledExtensionNames{ extensions.data() },
		};

		VkInstance instance{};
		vkCreateInstance(&instanceCI, nullptr, &instance);
		
		volkLoadInstance(instance);

		return instance;
	}

	VkPhysicalDevice getPhysicalDevice(VkInstance instance)
	{
		std::uint32_t physicalDeviceCount{};
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		return physicalDevices[0];
	}

	std::uint32_t getGraphicsQueueFamily(VkPhysicalDevice physicalDevice)
	{
		std::uint32_t queueFamilyPropertyCount{};
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

		for (int i{ 0 }; i < queueFamilyProperties.size(); ++i)
		{
			if (queueFamilyProperties[i].queueFlags || VK_QUEUE_GRAPHICS_BIT)
			{
				return i;
			}
		}

		throw std::exception{ "failed to find graphics queue family" };

		return 0;
	}

}