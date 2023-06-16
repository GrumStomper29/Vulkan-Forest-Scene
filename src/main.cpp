#include "instance.hpp"
#include "device.hpp"
#include "alloc.hpp"
#include "swapchain.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"
#include "frame.hpp"

#include "pipeline.hpp"

#include "mesh.hpp"

#include "volk/volk.h"
#include "GLFW/glfw3.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
		constexpr VkExtent2D    windowExtent                    { 800, 460 };
		const char*             windowTitle                     { "Vulkan Forest Scene" };
		constexpr int           preferredSwapchainImageCount    { 4 };
		constexpr VkFormat      swapchainImageFormat            { VK_FORMAT_B8G8R8A8_SRGB };

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		GLFWwindow* window{ glfwCreateWindow(windowExtent.width, windowExtent.height, windowTitle, nullptr, nullptr) };
		if (!window)
		{
			throw std::exception{ "failed to create window" };
		}

		VkInstance                  instance            { createInstance(true) };
		VkPhysicalDevice            physicalDevice      { getPhysicalDevice(instance) };
		std::uint32_t               graphicsQueueFamily { getGraphicsQueueFamily(physicalDevice) };
		VkQueue                     graphicsQueue{};
		VkDevice                    device              { createDevice(physicalDevice, graphicsQueueFamily, graphicsQueue) };
		VmaAllocator                allocator           { createAllocator(instance, physicalDevice, device) };
		VkSurfaceKHR                surface{};
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		VkSwapchainKHR              swapchain           { createSwapchain(device, surface, preferredSwapchainImageCount, swapchainImageFormat, windowExtent) };
		std::vector<VkImage>        swapchainImages     { getSwapchainImages(device, swapchain) };
		std::vector<VkImageView>    swapchainImageViews { createSwapchainImageViews(device, swapchainImages, swapchainImageFormat) };
		Image                       depthImage          { createDepthImage(allocator, windowExtent) };
		VkImageView                 depthImageView      { createDepthImageView(device, depthImage.image) };

		VkCommandPool				cmdPool				{ createCommandPool(device, graphicsQueueFamily, true) };
		VkCommandBuffer				mainCmdBuffer		{ allocateCommandBuffer(device, cmdPool) };
		VkFence						fence				{ createFence(device, false) };
		VkSemaphore					renderSemaphore		{ createSemaphore(device) };
		VkSemaphore					presentSemaphore	{ createSemaphore(device) };

		Frame frames[2]{};
		frames[0] =
		{
			.renderSemaphore{ createSemaphore(device) },
			.presentSemaphore{ createSemaphore(device) },
			.renderFence{ createFence(device, false) },
			.cmdPool{ createCommandPool(device, graphicsQueueFamily, true) },
		};
		frames[0].cmdBuffer = allocateCommandBuffer(device, frames[0].cmdPool);
		frames[1] =
		{
			.renderSemaphore{ createSemaphore(device) },
			.presentSemaphore{ createSemaphore(device) },
			.renderFence{ createFence(device, false) },
			.cmdPool{ createCommandPool(device, graphicsQueueFamily, true) },
		};
		frames[1].cmdBuffer = allocateCommandBuffer(device, frames[1].cmdPool);

		VkPipelineLayout            pipelineLayout      { createPipelineLayout(device) };
		VkPipeline                  pipeline            { createPipeline(device, pipelineLayout, windowExtent, swapchainImageFormat) };

		std::vector<Vertex>        vertices{};
		std::vector<std::uint32_t> indices{};
		std::vector<std::uint32_t> indices2{};
		loadOBJToVertices("assets/American Elm01 tree.obj", vertices, indices);
		loadOBJToVertices("assets/monkey.obj", vertices, indices2);

		const std::uint32_t vertexCount{ static_cast<std::uint32_t>(vertices.size()) };
		const std::uint32_t indexCount { static_cast<std::uint32_t>(indices.size()) };
		const std::uint32_t indexCount2{ static_cast<std::uint32_t>(indices2.size()) };

		Buffer vertexBuffer{ createVertexBuffer(vertices, device, allocator, graphicsQueue, mainCmdBuffer, fence) };
		Buffer indexBuffer { createIndexBuffer(indices, device, allocator, graphicsQueue, mainCmdBuffer, fence) };
		Buffer indexBuffer2{ createIndexBuffer(indices2, device, allocator, graphicsQueue, mainCmdBuffer, fence) };

		int frameNumber{ 0 };
		bool firstFrame{ true };

		while (!glfwWindowShouldClose(window))
		{
			std::uint32_t swapchainImageIndex{ acquireNextSwapchainImage(device, swapchain, frames[frameNumber].presentSemaphore) };

			vkResetCommandPool(device, frames[frameNumber].cmdPool, 0);

			beginCommandBuffer(frames[frameNumber].cmdBuffer, true);

			prepareImageForColorAttachmentOutput(frames[frameNumber].cmdBuffer, swapchainImages[swapchainImageIndex]);

			if (firstFrame)
			{
				correctDepthImageLayout(depthImage.image, frames[frameNumber].cmdBuffer);
			}

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

			VkRenderingAttachmentInfo depthAttachment
			{
				.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
				.imageView{ depthImageView },
				.imageLayout{ VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
				.resolveMode{ VK_RESOLVE_MODE_NONE },
				.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
				.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
				.clearValue{ .depthStencil{ .depth{ 1.0f } } },
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
				.pDepthAttachment{ &depthAttachment },
			};

			vkCmdBeginRendering(frames[frameNumber].cmdBuffer, &renderingInfo);

			vkCmdBindPipeline(frames[frameNumber].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			constexpr VkDeviceSize offset{ 0 };
			vkCmdBindVertexBuffers(frames[frameNumber].cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);

			vkCmdBindIndexBuffer(frames[frameNumber].cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			static std::uint64_t frame{ 0 };
			constexpr glm::vec3 camPos{ 0.0f, 1.0f, -3.0f };
			glm::mat4 view{ glm::translate(glm::mat4(1.0f), camPos) };
			constexpr float ar{ windowExtent.width / windowExtent.height };
			glm::mat4 proj{ glm::perspective(glm::radians(70.0f), ar, 0.1f, 200.0f) };
			glm::mat4 model{ glm::rotate(glm::mat4{ 1.0f }, glm::radians(frame++ * 0.04f), glm::vec3{ 0.0f, 1.0f, 0.0f }) };
			model = glm::scale(model, glm::vec3{ 0.001f });

			PushConstants pushConstants{ proj * view * model };
			vkCmdPushConstants(frames[frameNumber].cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

			vkCmdDrawIndexed(frames[frameNumber].cmdBuffer, indexCount, 1, 0, 0, 0);

			vkCmdBindIndexBuffer(frames[frameNumber].cmdBuffer, indexBuffer2.buffer, 0, VK_INDEX_TYPE_UINT32);

			model = glm::scale(model, glm::vec3{ 500.0f, 500.0f, 500.0f });
			pushConstants.vertexTransform = proj * view * model;
			vkCmdPushConstants(frames[frameNumber].cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

			vkCmdDrawIndexed(frames[frameNumber].cmdBuffer, indexCount2, 1, 0, 0, 0);

			vkCmdEndRendering(frames[frameNumber].cmdBuffer);

			prepareImageForPresentation(frames[frameNumber].cmdBuffer, swapchainImages[swapchainImageIndex]);

			vkEndCommandBuffer(frames[frameNumber].cmdBuffer);

			queueSubmit(graphicsQueue, frames[frameNumber].cmdBuffer, frames[frameNumber].presentSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frames[frameNumber].renderSemaphore, frames[frameNumber].renderFence);

			swapchainQueuePresent(graphicsQueue, swapchain, frames[frameNumber].renderSemaphore, swapchainImageIndex);

			vkWaitForFences(device, 1, &frames[frameNumber].renderFence, VK_TRUE, secondsToNanoseconds(1));
			vkResetFences(device, 1, &frames[frameNumber].renderFence);

			glfwPollEvents();

			frameNumber = ++frameNumber % 2;
			firstFrame = false;
		}

		vmaDestroyBuffer(allocator, indexBuffer2.buffer, indexBuffer2.alloc);
		vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.alloc);
		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.alloc);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		for (int i{ 0 }; i < 2; ++i)
		{
			vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
			vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
			vkDestroyFence(device, frames[i].renderFence, nullptr);
			vkDestroyCommandPool(device, frames[i].cmdPool, nullptr);
		}

		vkDestroySemaphore(device, presentSemaphore, nullptr);
		vkDestroySemaphore(device, renderSemaphore, nullptr);
		vkDestroyFence(device, fence, nullptr);
		vkDestroyCommandPool(device, cmdPool, nullptr);

		vkDestroyImageView(device, depthImageView, nullptr);
		vmaDestroyImage(allocator, depthImage.image, depthImage.alloc);
		for (VkImageView& swapchainImageView : swapchainImageViews)
		{
			vkDestroyImageView(device, swapchainImageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vmaDestroyAllocator(allocator);
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