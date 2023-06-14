#include "swapchain.hpp"

#include "sync.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <vector>

namespace Graphics
{

	VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, std::uint32_t minImageCount, VkFormat imageFormat, VkExtent2D imageExtent)
	{
		VkSwapchainCreateInfoKHR swapchainCI
		{
			.sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR },
			.surface{ surface },
			.minImageCount{ minImageCount },
			.imageFormat{ imageFormat },
			.imageColorSpace{ VK_COLOR_SPACE_SRGB_NONLINEAR_KHR  },
			.imageExtent{ imageExtent },
			.imageArrayLayers{ 1 },
			.imageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
			.imageSharingMode{ VK_SHARING_MODE_EXCLUSIVE },
			.preTransform{ VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR },
			.compositeAlpha{ VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR },
			.presentMode{ VK_PRESENT_MODE_MAILBOX_KHR },
			.clipped{ VK_TRUE },
		};

		VkSwapchainKHR swapchain{};
		vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain);

		return swapchain;
	}

	std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
	{
		std::uint32_t swapchainImageCount{};
		vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

		std::vector<VkImage> swapchainImages(swapchainImageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

		return swapchainImages;
	}

	std::vector<VkImageView> createSwapchainImageViews(VkDevice device, const std::vector<VkImage>& swapchainImages, VkFormat imageFormat)
	{
		std::vector<VkImageView> imageViews(swapchainImages.size());
		
		for (int i{ 0 }; i < imageViews.size(); ++i)
		{
			VkImageViewCreateInfo imageViewCI
			{
				.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
				.image{ swapchainImages[i] },
				.viewType{ VK_IMAGE_VIEW_TYPE_2D },
				.format{ imageFormat },
				.subresourceRange
					{
						.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
						.baseMipLevel{ 0 },
						.levelCount{ 1 },
						.baseArrayLayer{ 0 },
						.layerCount{ 1 },
					},
			};

			vkCreateImageView(device, &imageViewCI, nullptr, &imageViews[i]);
		}

		return imageViews;
	}

	std::uint32_t acquireNextSwapchainImage(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore signalSemaphore)
	{
		VkAcquireNextImageInfoKHR acquireInfo
		{
			.sType{ VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR },
			.swapchain{ swapchain },
			.timeout{ secondsToNanoseconds(1) },
			.semaphore{ signalSemaphore },
			.deviceMask{ 1 },
		};

		std::uint32_t imageIndex{};
		vkAcquireNextImage2KHR(device, &acquireInfo, &imageIndex);

		return imageIndex;
	}

	void prepareImageForColorAttachmentOutput(VkCommandBuffer commandBuffer, VkImage image)
	{
		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_NONE },
			.dstAccessMask{ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
			.newLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.image{ image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrier);
	}

	void prepareImageForPresentation(VkCommandBuffer commandBuffer, VkImage image)
	{
		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
			.dstAccessMask{ VK_ACCESS_NONE },
			.oldLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.newLayout{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			.image{ image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrier);
	}

	void swapchainQueuePresent(VkQueue queue, VkSwapchainKHR swapchain, VkSemaphore waitSemaphore, std::uint32_t imageIndex)
	{
		VkPresentInfoKHR presentInfo
		{
			.sType{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR },
			.waitSemaphoreCount{ 1 },
			.pWaitSemaphores{ &waitSemaphore },
			.swapchainCount{ 1 },
			.pSwapchains{ &swapchain },
			.pImageIndices{ &imageIndex },
		};

		vkQueuePresentKHR(queue, &presentInfo);
	}

}