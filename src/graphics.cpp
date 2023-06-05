#include "graphics.hpp"

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include "cmd_buffer.hpp"

#include "pipeline.hpp"

#include "image_transition.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "volk/volk.h"

#include <cstdlib>
#include <iostream>

// temp
#include <cstdint>

namespace Graphics
{

	// Note: The vast majority of initialization is done in the contructors of the objects created in Graphics::run().
	void init()
	{
		if (glfwInit() == GLFW_FALSE)
		{
			std::cerr << "error: failed to initialize GLFW.\n";
			std::abort();
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (volkInitialize() != VK_SUCCESS)
		{
			std::cerr << "error: failed to load Vulkan loader.\n";
			std::abort();
		}
	}

	void run()
	{
		constexpr int windowWidth{ 1920 };
		constexpr int windowHeight{ 1080 };

		// Core structures

		Window window{ windowWidth, windowHeight, "Vulkan-Forest-Scene" };
		Instance instance{ true };
		Device device{ instance };
		Swapchain swapchain{ window, instance, device, 4 };
		CmdBuffer graphicsBuffer{ device, device.graphicsQueueFamily() };

		Pipeline pipeline{ device, "shaders/uber_vertex.spv", "shaders/uber_fragment.spv", window.vkExtent() };

		// Synchronization structs
		VkSemaphore presentSemaphore{};
		VkSemaphore renderSemaphore{};
		VkFence renderFence{};

		VkSemaphoreCreateInfo semaphoreInfo{ .sType{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO } };
		vkCreateSemaphore(device.vkDevice(), &semaphoreInfo, nullptr, &presentSemaphore);
		vkCreateSemaphore(device.vkDevice(), &semaphoreInfo, nullptr, &renderSemaphore);

		VkFenceCreateInfo fenceInfo{ .sType{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO } };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(device.vkDevice(), &fenceInfo, nullptr, &renderFence);
		
		while (!window.shouldClose())
		{
			window.pollEvents();

			vkWaitForFences(device.vkDevice(), 1, &renderFence, VK_TRUE, 60000000000); // Wait up to one minute
			vkResetFences(device.vkDevice(), 1, &renderFence);

			std::uint32_t swapchainImageIndex{ swapchain.acquireNextImage(presentSemaphore) };

			graphicsBuffer.reset();
			graphicsBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			prepareImageForColorAttachmentOutput(swapchain.vkImages()[swapchainImageIndex], graphicsBuffer);

			VkRenderingAttachmentInfo colorAttachment{ .sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO } };
			colorAttachment.imageView = swapchain.vkImageViews()[swapchainImageIndex];
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

			VkRenderingInfo renderInfo{ .sType{ VK_STRUCTURE_TYPE_RENDERING_INFO } };
			renderInfo.renderArea = VkRect2D{ {}, window.vkExtent() };
			renderInfo.layerCount = 1;
			renderInfo.viewMask = 0;
			renderInfo.colorAttachmentCount = 1;
			renderInfo.pColorAttachments = &colorAttachment;

			vkCmdBeginRendering(graphicsBuffer.vkCommandBuffer(), &renderInfo);

			vkCmdBindPipeline(graphicsBuffer.vkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vkPipeline());

			vkCmdDraw(graphicsBuffer.vkCommandBuffer(), 3, 1, 0, 0);

			vkCmdEndRendering(graphicsBuffer.vkCommandBuffer());

			prepareImageForPresentation(swapchain.vkImages()[swapchainImageIndex], graphicsBuffer);

			graphicsBuffer.end();

			graphicsBuffer.submit({ presentSemaphore },
				{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
				{ renderSemaphore },
				device.graphicsQueue(),
				renderFence);

			swapchain.queuePresent({ renderSemaphore }, swapchainImageIndex);

			window.swapBuffers();
		}
		
		vkDeviceWaitIdle(device.vkDevice());

		vkDestroyFence(device.vkDevice(), renderFence, nullptr);
		vkDestroySemaphore(device.vkDevice(), presentSemaphore, nullptr);
		vkDestroySemaphore(device.vkDevice(), renderSemaphore, nullptr);
	}

	void cleanup()
	{
		glfwTerminate();
	}

}