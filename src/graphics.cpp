#include "graphics.hpp"

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include "cmd_buffer.hpp"

#include "pipeline.hpp"
#include "image_transition.hpp"

#include "mesh.hpp"
#include "buffer.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "volk/volk.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace Graphics
{

	struct PushConstants
	{
		glm::mat4 transform{};
	};

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

	Buffer loadVertices(std::vector<Vertex>& vertices, Device& device, CmdBuffer& cmdBuffer, VkFence fence)
	{
		const VkDeviceSize vertexBufferSize{ vertices.size() * sizeof(Vertex) };

		VkBufferCreateInfo stagingVertexBufferInfo
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ vertexBufferSize },
			.usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT }
		};

		VmaAllocationCreateInfo stagingVertexAllocInfo
		{
			.flags{ VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_HOST },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};

		Buffer stagingVertexBuffer{};
		vmaCreateBuffer(device.vmaAllocator(), &stagingVertexBufferInfo, &stagingVertexAllocInfo, &stagingVertexBuffer.buffer, &stagingVertexBuffer.alloc, nullptr);

		void* data{};
		vmaMapMemory(device.vmaAllocator(), stagingVertexBuffer.alloc, &data);
		std::memcpy(data, vertices.data(), vertexBufferSize);
		vmaUnmapMemory(device.vmaAllocator(), stagingVertexBuffer.alloc);

		vertices.clear();
		vertices.shrink_to_fit();

		VkBufferCreateInfo vertexBufferInfo
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ vertexBufferSize },
			.usage{ VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT }
		};

		VmaAllocationCreateInfo vertexAllocInfo
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.preferredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
		};

		Buffer vertexBuffer{};
		vmaCreateBuffer(device.vmaAllocator(), &vertexBufferInfo, &vertexAllocInfo, &vertexBuffer.buffer, &vertexBuffer.alloc, nullptr);

		cmdBuffer.reset();
		cmdBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkBufferCopy region{ .size{ vertexBufferSize } };
		vkCmdCopyBuffer(cmdBuffer.vkCommandBuffer(), stagingVertexBuffer.buffer, vertexBuffer.buffer, 1, &region);

		cmdBuffer.end();
		cmdBuffer.submit({  },
			{  },
			{  },
			device.graphicsQueue(),
			fence);

		vkWaitForFences(device.vkDevice(), 1, &fence, VK_TRUE, 600000000000); // 10 minutes
		vkResetFences(device.vkDevice(), 1, &fence);

		vmaDestroyBuffer(device.vmaAllocator(), stagingVertexBuffer.buffer, stagingVertexBuffer.alloc);

		return vertexBuffer;
	}

	glm::mat4 calculateTransform(int frameNumber)
	{
		constexpr glm::vec3 camPos{ 0.0f, 0.0f, -2.0f };

		glm::mat4 view{ glm::translate(glm::mat4(1.0f), camPos) };

		glm::mat4 proj{ glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.1f, 200.0f) };

		glm::mat4 model{ glm::rotate(glm::mat4{ 1.0f }, glm::radians(frameNumber * 0.04f), glm::vec3{ 0.0f, 1.0f, 0.0f }) };

		model *= 0.01f;

		return proj * view * model;
	}

	void run()
	{
		constexpr bool enableFullscreen{ false };

		unsigned int windowWidth{ 1920 };
		unsigned int windowHeight{ 1080 };

		if (!enableFullscreen)
		{
			windowHeight /= 2;
			windowWidth /= 2;
		}

		// Core structures
		Window window{ windowWidth, windowHeight, "Vulkan-Forest-Scene", enableFullscreen };
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
		vkCreateFence(device.vkDevice(), &fenceInfo, nullptr, &renderFence);

		// Meshes
		std::vector<Vertex> vertices{};
		auto indices{ loadOBJ("assets/american_elm_tree_obj/American Elm01 tree.obj", vertices) };
		//auto indices{ loadOBJ("assets/monkey_smooth.obj", vertices) };
		const int vertexCount{ static_cast<int>(vertices.size()) };
		const VkDeviceSize vertexBufferSize{ vertices.size() * sizeof(Vertex) };

		Buffer vertexBuffer{ loadVertices(vertices, device, graphicsBuffer, renderFence) };

		int frameNumber{ 0 };

		while (!window.shouldClose())
		{
			window.pollEvents();

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

			PushConstants pushConstants{ .transform{ calculateTransform(++frameNumber) } };
			vkCmdPushConstants(graphicsBuffer.vkCommandBuffer(), pipeline.vkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

			VkDeviceSize offset{ 0 };
			vkCmdBindVertexBuffers(graphicsBuffer.vkCommandBuffer(), 0, 1, &vertexBuffer.buffer, &offset);

			vkCmdDraw(graphicsBuffer.vkCommandBuffer(), vertexCount, 1, 0, 0);

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

			vkWaitForFences(device.vkDevice(), 1, &renderFence, VK_TRUE, 60000000000); // Wait up to one minute
			vkResetFences(device.vkDevice(), 1, &renderFence);
		}

		vkDeviceWaitIdle(device.vkDevice());

		vmaDestroyBuffer(device.vmaAllocator(), vertexBuffer.buffer, vertexBuffer.alloc);

		vkDestroyFence(device.vkDevice(), renderFence, nullptr);
		vkDestroySemaphore(device.vkDevice(), presentSemaphore, nullptr);
		vkDestroySemaphore(device.vkDevice(), renderSemaphore, nullptr);
	}

	void cleanup()
	{
		glfwTerminate();
	}

}