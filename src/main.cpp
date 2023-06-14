#include "instance.hpp"
#include "device.hpp"
#include "swapchain.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"

#include "pipeline.hpp"

#include "volk/volk.h"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <vector>

namespace Graphics
{

	void init()
	{
		if (!glfwInit())
		{
			throw std::exception{ "failed to initialize GLFW" };
		}

		if (volkInitialize() != VK_SUCCESS)
		{
			throw std::exception{ "failed to initialize volk" };
		}
	}

	void run()
	{
		constexpr VkExtent2D    windowExtent                    { 640, 480 };
		const char*             windowTitle                     { "Vulkan Forest Scene" };
		constexpr int           preferredSwapchainImageCount    { 4 };
		constexpr VkFormat      swapchainImageFormat            { VK_FORMAT_B8G8R8A8_SRGB };

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window{ glfwCreateWindow(windowExtent.width, windowExtent.height, windowTitle, nullptr, nullptr) };
		if (!window)
		{
			throw std::exception{ "failed to create window" };
		}

		VkInstance                  instance            { Graphics::createInstance(true) };
		VkPhysicalDevice            physicalDevice      { Graphics::getPhysicalDevice(instance) };
		std::uint32_t               graphicsQueueFamily { Graphics::getGraphicsQueueFamily(physicalDevice) };
		VkQueue                     graphicsQueue{};
		VkDevice                    device              { Graphics::createDevice(physicalDevice, graphicsQueueFamily, graphicsQueue) };
		VkSurfaceKHR                surface{};
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		VkSwapchainKHR              swapchain           { Graphics::createSwapchain(device, surface, preferredSwapchainImageCount, swapchainImageFormat, windowExtent) };
		std::vector<VkImage>        swapchainImages     { Graphics::getSwapchainImages(device, swapchain) };
		std::vector<VkImageView>    swapchainImageViews { Graphics::createSwapchainImageViews(device, swapchainImages, swapchainImageFormat) };

		VkCommandPool				cmdPool				{ Graphics::createCommandPool(device, graphicsQueueFamily, true) };
		VkCommandBuffer				mainCmdBuffer		{ Graphics::allocateCommandBuffer(device, cmdPool) };
		VkFence						fence				{ Graphics::createFence(device, false) };
		VkSemaphore					renderSemaphore		{ Graphics::createSemaphore(device) };
		VkSemaphore					presentSemaphore	{ Graphics::createSemaphore(device) };

		VkPipelineLayout            pipelineLayout      { createPipelineLayout(device) };
		VkPipeline                  pipeline            { createPipeline(device, pipelineLayout, windowExtent, swapchainImageFormat) };

		while (!glfwWindowShouldClose(window))
		{
			std::uint32_t swapchainImageIndex{ Graphics::acquireNextSwapchainImage(device, swapchain, presentSemaphore) };

			vkResetCommandBuffer(mainCmdBuffer, 0);

			beginCommandBuffer(mainCmdBuffer, true);
			
			Graphics::prepareImageForColorAttachmentOutput(mainCmdBuffer, swapchainImages[swapchainImageIndex]);

			VkRenderingAttachmentInfo colorAttachment
			{
				.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
				.imageView{ swapchainImageViews[swapchainImageIndex] },
				.imageLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
				.resolveMode{ VK_RESOLVE_MODE_NONE },
				.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
				.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
				.clearValue
				{
					.color{ 0.6f, 0.0f, 0.9f, 1.0f },
				},
			};

			VkRenderingInfo renderingInfo
			{
				.sType{ VK_STRUCTURE_TYPE_RENDERING_INFO },
				.renderArea
				{
					.offset{ 0, 0 },
					.extent{ windowExtent },
				},
				.layerCount{ 1 },
				.viewMask{ 0 },
				.colorAttachmentCount{ 1 },
				.pColorAttachments{ &colorAttachment },
			};

			vkCmdBeginRendering(mainCmdBuffer, &renderingInfo);

			vkCmdBindPipeline(mainCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			vkCmdDraw(mainCmdBuffer, 3, 1, 0, 0);

			vkCmdEndRendering(mainCmdBuffer);

			Graphics::prepareImageForPresentation(mainCmdBuffer, swapchainImages[swapchainImageIndex]);

			vkEndCommandBuffer(mainCmdBuffer);

			// Because of image transitions I have to wait until presentation is finished before the pipeline can begin
			queueSubmit(graphicsQueue, mainCmdBuffer, presentSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, renderSemaphore, fence);

			swapchainQueuePresent(graphicsQueue, swapchain, renderSemaphore, swapchainImageIndex);

			vkWaitForFences(device, 1, &fence, VK_TRUE, Graphics::secondsToNanoseconds(1));
			vkResetFences(device, 1, &fence);

			glfwPollEvents();
		}

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		vkDestroySemaphore(device, presentSemaphore, nullptr);
		vkDestroySemaphore(device, renderSemaphore, nullptr);
		vkDestroyFence(device, fence, nullptr);
		vkDestroyCommandPool(device, cmdPool, nullptr);

		for (VkImageView& swapchainImageView : swapchainImageViews)
		{
			vkDestroyImageView(device, swapchainImageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
	}

	void cleanup()
	{
		glfwTerminate();
	}

}

int main()
{
	Graphics::init();
	Graphics::run();
	Graphics::cleanup();

	return 0;
}