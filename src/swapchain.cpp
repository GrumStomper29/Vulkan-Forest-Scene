#include "swapchain.hpp"

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "cmd_buffer.hpp"

#include "image.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Graphics
{

	Image createDepthImage(VmaAllocator allocator, VkExtent3D extent)
	{
		VkImageCreateInfo imageInfo
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
			.imageType{ VK_IMAGE_TYPE_2D },
			.format{ VK_FORMAT_D32_SFLOAT },
			.extent{ extent },
			.mipLevels{ 1 },
			.arrayLayers{ 1 },
			.samples{ VK_SAMPLE_COUNT_1_BIT },
			.tiling{ VK_IMAGE_TILING_OPTIMAL },
			.usage{ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT },
			.initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED }
		};

		VmaAllocationCreateInfo allocInfo
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
		};

		Image depthImage{};
		vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage.image, &depthImage.alloc, nullptr);

		return depthImage;
	}

	VkImageView createDepthImageView(VkDevice device, VkImage image)
	{
		VkImageViewCreateInfo imageViewInfo
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image{ image },
			.viewType{ VK_IMAGE_VIEW_TYPE_2D },
			.format{ VK_FORMAT_D32_SFLOAT },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_DEPTH_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		VkImageView imageView{};
		vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);

		return imageView;
	}

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

		VkImageViewCreateInfo imageViewInfo{ .sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO } };
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		// We will use default component mapping for now. Maybe we can screw with this later.
		imageViewInfo.subresourceRange =
		{
			.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
			.baseMipLevel{ 0 },
			.levelCount{ 1 },
			.baseArrayLayer{ 0 },
			.layerCount{ 1 }
		};

		m_imageViews.resize(m_images.size());

		for (int i{ 0 }; i < m_images.size(); ++i)
		{
			imageViewInfo.image = m_images[i];
			vkCreateImageView(m_device.vkDevice(), &imageViewInfo, nullptr, &m_imageViews[i]);
		}

		m_depthImage = createDepthImage(m_device.vmaAllocator(), VkExtent3D{ window.vkExtent().width, window.vkExtent().height, 1 });
		m_depthImageView = createDepthImageView(m_device.vkDevice(), m_depthImage.image);
	}

	Swapchain::~Swapchain()
	{
		vkDestroyImageView(m_device.vkDevice(), m_depthImageView, nullptr);
		vmaDestroyImage(m_device.vmaAllocator(), m_depthImage.image, m_depthImage.alloc);

		for (auto view : m_imageViews)
		{
			vkDestroyImageView(m_device.vkDevice(), view, nullptr);
		}
		vkDestroySwapchainKHR(m_device.vkDevice(), m_swapchain, nullptr);

		vkDestroySurfaceKHR(m_instance.vkInstance(), m_surface, nullptr);
	}

}