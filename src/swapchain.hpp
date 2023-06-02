#pragma once

#include "volk/volk.h"

#include "instance.hpp"
#include "window.hpp"
#include "device.hpp"
#include "cmd_buffer.hpp"

#include <cstdint>
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

		VkSwapchainKHR& vkSwapchainKHR() noexcept
		{
			return m_swapchain;
		}
		VkSwapchainKHR vkSwapchainKHR() const noexcept
		{
			return m_swapchain;
		}

		const std::vector<VkImage>& vkImages() const noexcept
		{
			return m_images;
		}

		const std::vector<VkImageView>& vkImageViews() const noexcept
		{
			return m_imageViews;
		}

		std::uint32_t acquireNextImage(VkSemaphore semaphore) const
		{
			std::uint32_t imageIndex{};
			vkAcquireNextImageKHR(m_device.vkDevice(), m_swapchain, 60000000000, semaphore, VK_NULL_HANDLE, &imageIndex);
			return imageIndex;
		}

		VkResult queuePresent(const std::vector<VkSemaphore>& waitSemaphores, std::uint32_t imageIndex)
		{
			VkPresentInfoKHR presentInfo{ .sType{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR } };
			presentInfo.waitSemaphoreCount = waitSemaphores.size();
			presentInfo.pWaitSemaphores = waitSemaphores.data();
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_swapchain;
			presentInfo.pImageIndices = &imageIndex;

			return vkQueuePresentKHR(m_device.graphicsQueue(), &presentInfo);
		}

	private:
		VkSurfaceKHR m_surface{};
		VkSwapchainKHR m_swapchain{};

		const Instance& m_instance;
		const Device& m_device;

		std::vector<VkImage> m_images{};
		std::vector<VkImageView> m_imageViews{};
	};

}