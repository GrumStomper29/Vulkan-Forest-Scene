#include "swapchain.hpp"

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "window.hpp"
#include "instance.hpp"

#include <stdexcept>

namespace Graphics
{

	Swapchain::Swapchain(const Instance& instance, const Window& window)
		: m_instance{ instance.getVkInstanceHandle() }
	{
		VkResult result{ glfwCreateWindowSurface(m_instance, window.getGLFWwindowHandle(), nullptr, &m_surface) };

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: GLFW failed to create VkSurfaceKHR.\n" };
		}

		VkSwapchainCreateInfoKHR swapchainInfo{ .sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR } };
		swapchainInfo.surface = m_surface;
		// TODO: FINISH SWAPCHAIN CREATION
	}

	Swapchain::~Swapchain()
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	}

}