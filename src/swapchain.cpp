#include "swapchain.hpp"

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"

#include <cstdint>
#include <stdexcept>

namespace Graphics
{

	Swapchain::Swapchain(const Window& window, const Instance& instance, const Device& device, std::uint32_t minImageCount)
		: m_instance{ instance }
		, m_device{ device }
	{
		VkResult result{ glfwCreateWindowSurface(m_instance.vkInstance(), window.glfwWindow(), nullptr, &m_surface)};

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: GLFW failed to create VkSurfaceKHR.\n" };
		}

		VkSwapchainCreateInfoKHR swapchainInfo{ .sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR } };
		swapchainInfo.surface = m_surface;
		swapchainInfo.minImageCount = minImageCount;
		swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapchainInfo.imageExtent = window.vkExtent();
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		swapchainInfo.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(m_device.vkDevice(), &swapchainInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error{ "error: Vulkan failed to create VkSwapchainKHR.\n" };
		}

		std::uint32_t swapchainImageCount{};
		vkGetSwapchainImagesKHR(m_device.vkDevice(), m_swapchain, &swapchainImageCount, nullptr);

		m_images.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(m_device.vkDevice(), m_swapchain, &swapchainImageCount, m_images.data());
	}

	Swapchain::~Swapchain()
	{
		vkDestroySwapchainKHR(m_device.vkDevice(), m_swapchain, nullptr);
		vkDestroySurfaceKHR(m_instance.vkInstance(), m_surface, nullptr);
	}

}