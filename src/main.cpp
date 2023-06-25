#include "instance.hpp"
#include "device.hpp"
#include "alloc.hpp"
#include "swapchain.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"

#include "frame.hpp"

#include "descriptor.hpp"

#include "mesh.hpp"

#include "pipeline.hpp"

#include "camera.hpp"

#include "volk/volk.h"
#include "GLFW/glfw3.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdint>   // For std::memcpy
#include <cstring>   // For std::uint32_t
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
		constexpr VkExtent2D    windowExtent{ 1600, 900 };
		const char*             windowTitle{ "Vulkan Forest Scene" };
		constexpr int           preferredSwapchainImageCount{ 4 };
		constexpr VkFormat      swapchainImageFormat{ VK_FORMAT_B8G8R8A8_SRGB };


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

		VkCommandPool   GPCmdPool   { createCommandPool(device, graphicsQueueFamily, true) };
		VkCommandBuffer GPCmdBuffer { allocateCommandBuffer(device, GPCmdPool) };
		VkFence         GPFence     { createFence(device, false) };

		Frame::init(device);
		std::vector<Frame> framesInFlight{};
		framesInFlight.reserve(2);
		framesInFlight.push_back({ device, allocator, graphicsQueueFamily });
		framesInFlight.push_back({ device, allocator, graphicsQueueFamily });

		std::vector<Vertex> vertices{};

		std::vector<RenderObject> renderObjects{};
		renderObjects.reserve(2);
		renderObjects.push_back({ "assets/American Elm01 tree.obj", vertices, device, allocator, graphicsQueue, GPCmdBuffer, GPFence });
		renderObjects.push_back({ "assets/forest.obj", vertices, device, allocator, graphicsQueue, GPCmdBuffer, GPFence });

		Buffer vertexBuffer{ createVertexBuffer(vertices, device, allocator, graphicsQueue, GPCmdBuffer, GPFence) };

		std::vector<Texture> textures{};
		for (auto& r : renderObjects)
		{
			for (auto& m : r.meshes)
			{
				m.textureIndex = textures.size();
				textures.push_back({ m.diffusePath.c_str(), device, allocator, graphicsQueue, GPCmdBuffer, GPFence });
			}
		}

		VkDescriptorPool descriptorPool           { createDescriptorPool(device) };
		VkDescriptorSetLayout descriptorSetLayout { createDescriptorSetLayout(device) };
		VkDescriptorSet descriptorSet             { allocateDescriptorSet( device,  descriptorPool,  descriptorSetLayout) };

		std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
		std::vector<VkWriteDescriptorSet> writes(textures.size());
		for (int i{ 0 }; i < textures.size(); ++i)
		{
			imageInfos[i] =
			{
				.sampler{ textures[i].vkSampler() },
				.imageView{ textures[i].vkImageView() },
				.imageLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			};
			writes[i] =
			{
				.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
				.dstSet{ descriptorSet },
				.dstBinding{ 0 },
				.dstArrayElement{ static_cast<std::uint32_t>(i) },
				.descriptorCount{ 1 },
				.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
				.pImageInfo{ &imageInfos[i] },
			};
		}
		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

		VkDescriptorSetLayout setLayouts[2]
		{
			Frame::getDescriptorSetLayout(),
			descriptorSetLayout
		};
		VkPipelineLayout            pipelineLayout{ createPipelineLayout(device, 2, setLayouts) };
		VkPipeline                  pipeline{ createPipeline(device, pipelineLayout, windowExtent, swapchainImageFormat) };

		Camera camera{ {0.0f, -3.0f, -10.0f }, { 0.0f, -90.0f } };

		int frameNumber{ 0 };
		bool firstFrame{ true };

		float lastFrame{ 0.0f };
		float deltaTime{ 0.0f };

		const glm::mat4 proj{ glm::perspective(glm::radians(90.0f), static_cast<float>(windowExtent.width) / windowExtent.height, 0.1f, 20000.0f) };

		while (!glfwWindowShouldClose(window))
		{
			float current{ static_cast<float>(glfwGetTime()) };
			deltaTime = current - lastFrame;
			lastFrame = current;

			float camMoveSpeed{ 6.0f * deltaTime };
			float camRotateSpeed{ 100.0f * deltaTime };

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camMoveSpeed *= 100.0f;

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

			glm::mat4 model{ 1.0f };
			model = glm::scale(model, glm::vec3{ 0.01f });
			model = glm::translate(model, { 0.0f, -150.0f, 0.0f });
			renderObjects[0].transform = model;
			renderObjects[0].meshes[1].draw = false;

			model = glm::mat4{ 1.0f };
			model = glm::scale(model, glm::vec3{ 7.5f });
			renderObjects[1].transform = model;

			CameraUBOData ubo{ .viewProj{ proj * camera.getViewMatrix() }};

			std::memcpy(framesInFlight[frameNumber].cameraUBOData, &ubo, sizeof(CameraUBOData));

			framesInFlight[frameNumber].execute(graphicsQueue, swapchain, windowExtent,
				swapchainImages, swapchainImageViews,
				depthImage, depthImageView,
				firstFrame, pipeline, pipelineLayout,
				vertexBuffer, renderObjects, descriptorSet);

			glfwPollEvents();

			frameNumber = ++frameNumber % 2;
			firstFrame = false;
		}

		vkDeviceWaitIdle(device);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		textures.clear();
		renderObjects.clear();

		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.alloc);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		framesInFlight.clear();
		Frame::cleanup(device);

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