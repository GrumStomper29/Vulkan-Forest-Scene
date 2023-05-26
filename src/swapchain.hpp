#pragma once

#include "volk/volk.h"

#include "instance.hpp"
#include "window.hpp"
#include "device.hpp"

#include <vector>

namespace Graphics
{

	class Swapchain final
	{
	public:
		Swapchain(const Window& window, const Instance& instance, const Device& device, std::uint32_t minImageCount);

		Swapchain(Swapchain&) = delete;
		Swapchain& operator=(Swapchain&) = delete;

		~Swapchain();
		
		VkSurfaceKHR vkSurfaceKHR() const noexcept
		{
			return m_surface;
		}

		VkSwapchainKHR vkSwapchainKHR() const noexcept
		{
			return m_swapchain;
		}

	private:
		VkSurfaceKHR m_surface{};
		VkSwapchainKHR m_swapchain{};

		const Instance& m_instance;
		const Device& m_device;

		std::vector<VkImage> m_images{};
	};

}