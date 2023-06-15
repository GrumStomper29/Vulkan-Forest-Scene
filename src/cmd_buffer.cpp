#include "cmd_buffer.hpp"

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	VkCommandPool createCommandPool(VkDevice device, std::uint32_t queueFamily, bool resettable)
	{
		VkCommandPoolCreateInfo commandPoolCI
		{
			.sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
			.flags{ resettable ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : static_cast<VkCommandPoolCreateFlags>(0) },
			.queueFamilyIndex{ queueFamily },
		};

		VkCommandPool commandPool{};
		vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool);

		return commandPool;
	}

	VkCommandBuffer allocateCommandBuffer(VkDevice device, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo commandBufferAI
		{
			.sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO },
			.commandPool{ commandPool },
			.level{ VK_COMMAND_BUFFER_LEVEL_PRIMARY },
			.commandBufferCount{ 1 },
		};

		VkCommandBuffer commandBuffer{};
		vkAllocateCommandBuffers(device, &commandBufferAI, &commandBuffer);

		return commandBuffer;
	}

	void beginCommandBuffer(VkCommandBuffer commandBuffer, bool oneTimeSubmit)
	{
		VkCommandBufferBeginInfo beginInfo
		{
			.sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
			.flags{ oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : static_cast<VkCommandBufferUsageFlags>(0) },
		};

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	void queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage, VkSemaphore signalSemaphore, VkFence fence)
	{
		VkSubmitInfo submitInfo
		{
			.sType{ VK_STRUCTURE_TYPE_SUBMIT_INFO },
			.waitSemaphoreCount{ (waitSemaphore == VK_NULL_HANDLE) ? 0u : 1u },
			.pWaitSemaphores{ &waitSemaphore },
			.pWaitDstStageMask{ &waitStage },
			.commandBufferCount{ 1 },
			.pCommandBuffers{ &commandBuffer },
			.signalSemaphoreCount{ (signalSemaphore == VK_NULL_HANDLE) ? 0u : 1u },
			.pSignalSemaphores{ &signalSemaphore },
		};

		vkQueueSubmit(queue, 1, &submitInfo, fence);
	}

}