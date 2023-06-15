#include "instance.hpp"
#include "device.hpp"
#include "alloc.hpp"
#include "swapchain.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"

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
		constexpr VkExtent2D    windowExtent                    { 640, 480 };
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

		VkPipelineLayout            pipelineLayout      { createPipelineLayout(device) };
		VkPipeline                  pipeline            { createPipeline(device, pipelineLayout, windowExtent, swapchainImageFormat) };

		std::vector<Vertex>        vertices{};
		std::vector<std::uint32_t> indices{};
		loadOBJToVertices("assets/American Elm01 tree.obj", vertices, indices);

		const std::uint32_t vertexCount{ static_cast<std::uint32_t>(vertices.size()) };

		Buffer vertexBuffer{ createVertexBuffer(vertices, device, allocator, graphicsQueue, mainCmdBuffer, fence) };

		while (!glfwWindowShouldClose(window))
		{
			std::uint32_t swapchainImageIndex{ acquireNextSwapchainImage(device, swapchain, presentSemaphore) };

			vkResetCommandBuffer(mainCmdBuffer, 0);

			beginCommandBuffer(mainCmdBuffer, true);
			
			prepareImageForColorAttachmentOutput(mainCmdBuffer, swapchainImages[swapchainImageIndex]);
			correctDepthImageLayout(depthImage.image, mainCmdBuffer);

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

			vkCmdBeginRendering(mainCmdBuffer, &renderingInfo);

			vkCmdBindPipeline(mainCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			constexpr VkDeviceSize offset{ 0 };
			vkCmdBindVertexBuffers(mainCmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);

			static std::uint64_t frame{ 0 };
			constexpr glm::vec3 camPos{ 0.0f, 1.0f, -3.0f };
			glm::mat4 view{ glm::translate(glm::mat4(1.0f), camPos) };
			constexpr float ar{ windowExtent.width / windowExtent.height };
			glm::mat4 proj{ glm::perspective(glm::radians(70.0f), ar, 0.1f, 200.0f) };
			glm::mat4 model{ glm::rotate(glm::mat4{ 1.0f }, glm::radians(frame++ * 0.04f), glm::vec3{ 0.0f, 1.0f, 0.0f }) };
			model = glm::scale(model, glm::vec3{ 0.001f });

			PushConstants pushConstants{ proj * view * model };
			vkCmdPushConstants(mainCmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

			vkCmdDraw(mainCmdBuffer, vertexCount, 1, 0, 0);

			vkCmdEndRendering(mainCmdBuffer);

			prepareImageForPresentation(mainCmdBuffer, swapchainImages[swapchainImageIndex]);

			vkEndCommandBuffer(mainCmdBuffer);

			// Because of image transitions I have to wait until presentation is finished before the pipeline can begin
			queueSubmit(graphicsQueue, mainCmdBuffer, presentSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, renderSemaphore, fence);

			swapchainQueuePresent(graphicsQueue, swapchain, renderSemaphore, swapchainImageIndex);

			vkWaitForFences(device, 1, &fence, VK_TRUE, secondsToNanoseconds(1));
			vkResetFences(device, 1, &fence);

			glfwPollEvents();
		}

		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.alloc);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

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