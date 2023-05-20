#include "instance.hpp"

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Graphics
{

	Instance::Instance()
	{
		VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.apiVersion = VK_API_VERSION_1_3;

		std::vector<const char*> extensions;
		
		std::uint32_t extensionCount{};
		const char** glfwExtensionsBegin{ glfwGetRequiredInstanceExtensions(&extensionCount) };
		extensions.insert(extensions.end(), glfwExtensionsBegin, glfwExtensionsBegin + extensionCount);

		VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledExtensionCount = extensionCount;
		instanceInfo.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: failed to create Vulkan instance\n" };
		}

		volkLoadInstance(m_instance);
	}

}