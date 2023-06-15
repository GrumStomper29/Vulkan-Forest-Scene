#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

#include <cstdint>
#include <vector>

namespace Graphics
{

	VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, std::uint32_t minImageCount, VkFormat imageFormat, VkExtent2D imageExtent);

	std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);

	std::vector<VkImageView> createSwapchainImageViews(VkDevice device, const std::vector<VkImage>& swapchainImages, VkFormat imageFormat);

	std::uint32_t acquireNextSwapchainImage(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore signalSemaphore);

	void prepareImageForColorAttachmentOutput(VkCommandBuffer commandBuffer, VkImage image);

	void prepareImageForPresentation(VkCommandBuffer commandBuffer, VkImage image);

	void swapchainQueuePresent(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore waitSemaphore, std::uint32_t imageIndex);

	Image createDepthImage(VmaAllocator allocator, VkExtent2D extent);

	VkImageView createDepthImageView(VkDevice device, VkImage depthImage);

	void correctDepthImageLayout(VkImage image, VkCommandBuffer commandBuffer);

}