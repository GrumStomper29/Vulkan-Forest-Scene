#include "instance.hpp"
#include "device.hpp"
#include "alloc.hpp"
#include "swapchain.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"
#include "frame.hpp"

#include "pipeline.hpp"

#include "mesh.hpp"

#include "camera.hpp"

#include "volk/volk.h"
#include "GLFW/glfw3.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdint>
#include <cstring>
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

		VkCommandPool				GPCmdPool           { createCommandPool(device, graphicsQueueFamily, true) };
		VkCommandBuffer				GPCmdBuffer         { allocateCommandBuffer(device, GPCmdPool) };
		VkFence						GPFence             { createFence(device, false) };

		std::vector<Frame> framesInFlight{};
		framesInFlight.reserve(2);
		framesInFlight.push_back({ device, allocator, graphicsQueueFamily });
		framesInFlight.push_back({ device, allocator, graphicsQueueFamily });

		VkDescriptorSetLayout setLayouts[2]
		{
			framesInFlight[0].getDescriptorSetLayout(),
			framesInFlight[1].getDescriptorSetLayout()
		};

		VkPipelineLayout            pipelineLayout      { createPipelineLayout(device, setLayouts) };
		VkPipeline                  pipeline            { createPipeline(device, pipelineLayout, windowExtent, swapchainImageFormat) };

		std::vector<Vertex>        vertices{};
		std::vector<std::uint32_t> indices{};
		std::vector<std::uint32_t> indices2{};
		loadOBJToVertices("assets/American Elm01 tree.obj", vertices, indices);
		loadOBJToVertices("assets/monkey.obj", vertices, indices2);

		std::vector<Renderable> renderables(2);
		renderables[0].indexCount = static_cast<std::uint32_t>(indices.size());
		renderables[1].indexCount = static_cast<std::uint32_t>(indices2.size());

		Buffer vertexBuffer{ createVertexBuffer(vertices, device, allocator, graphicsQueue, GPCmdBuffer, GPFence) };
		renderables[0].indexBuffer = createIndexBuffer(indices, device, allocator, graphicsQueue, GPCmdBuffer, GPFence);
		renderables[1].indexBuffer = createIndexBuffer(indices2, device, allocator, graphicsQueue, GPCmdBuffer, GPFence);

		Camera camera{ {0.0f, -3.0f, -10.0f }, { 0.0f, -90.0f } };

		int frameNumber{ 0 };
		bool firstFrame{ true };

		float lastFrame{ 0.0f };
		float deltaTime{ 0.0f };

		const glm::mat4 proj{ glm::perspective(glm::radians(90.0f), static_cast<float>(windowExtent.width) / windowExtent.height, 0.1f, 200.0f) };

		while (!glfwWindowShouldClose(window))
		{
			float current{ static_cast<float>(glfwGetTime()) };
			deltaTime = current - lastFrame;
			lastFrame = current;

			float camMoveSpeed{ 6.0f * deltaTime };
			float camRotateSpeed{ 100.0f * deltaTime };

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(0.0f, 0.0f, -camMoveSpeed);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(0.0f, 0.0f, camMoveSpeed);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(-camMoveSpeed, 0.0f, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(camMoveSpeed, 0.0f, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.move(0.0f, -camMoveSpeed, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.move(0.0f, camMoveSpeed, 0.0f);

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    camera.rotate(camRotateSpeed, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  camera.rotate(-camRotateSpeed, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  camera.rotate(0.0f, -camRotateSpeed);
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotate(0.0f, camRotateSpeed);

			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			{
				std::cout << camera.rotation().y << '\n';
			}

			glm::mat4 model{ 1.0f };
			model = glm::scale(model, glm::vec3{ 0.01f });

			PushConstants pushConstants{ .vertexTransform{ model } };
			CameraUBOData ubo{ .viewProj{ proj * camera.getViewMatrix() }};

			std::memcpy(framesInFlight[frameNumber].cameraUBOData, &ubo, sizeof(CameraUBOData));

			framesInFlight[frameNumber].execute(graphicsQueue, swapchain, windowExtent,
				swapchainImages, swapchainImageViews,
				depthImage, depthImageView,
				firstFrame, pipeline, pipelineLayout,
				vertexBuffer, renderables,
				pushConstants);

			glfwPollEvents();

			frameNumber = ++frameNumber % 2;
			firstFrame = false;
		}

		vkDeviceWaitIdle(device);

		for (const auto& r : renderables)
		{
			vmaDestroyBuffer(allocator, r.indexBuffer.buffer, r.indexBuffer.alloc);
		}

		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.alloc);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		framesInFlight.clear();

		vkDestroyFence(device, GPFence, nullptr);
		vkDestroyCommandPool(device, GPCmdPool, nullptr);

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