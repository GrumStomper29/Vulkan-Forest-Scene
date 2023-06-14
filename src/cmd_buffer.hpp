#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	VkCommandPool createCommandPool(VkDevice device, std::uint32_t queueFamily, bool resettable);

	VkCommandBuffer allocateCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void beginCommandBuffer(VkCommandBuffer commandBuffer, bool oneTimeSubmit);

	void queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage, VkSemaphore signalSemaphore, VkFence fence);
}