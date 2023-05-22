#pragma once

#include "volk/volk.h"

#include "instance.hpp"
#include "window.hpp"

namespace Graphics
{

	class Swapchain final
	{
	public:
		Swapchain(const Instance& instance, const Window& window);

		Swapchain(Swapchain&) = delete;
		Swapchain& operator=(Swapchain&) = delete;

		~Swapchain();
		
	private:
		VkSurfaceKHR m_surface{};
		VkSwapchainKHR m_swapchain{};

		VkInstance m_instance{};
	};

}