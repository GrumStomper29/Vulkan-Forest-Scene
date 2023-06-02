#include "image_transition.hpp"

#include "cmd_buffer.hpp"

#include "volk/volk.h"

namespace Graphics
{

	void prepareImageForColorAttachmentOutput(VkImage image, const CmdBuffer& cmdBuffer)
	{
		VkImageMemoryBarrier2 imgBarrier{ .sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 } };
		imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		imgBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier.image = image;
		imgBarrier.subresourceRange = {
			.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
			.baseMipLevel{ 0 },
			.levelCount{ 1 },
			.baseArrayLayer{ 0 },
			.layerCount{ 1 }
		};

		VkDependencyInfo dependencyInfo{ .sType{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO } };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imgBarrier;

		vkCmdPipelineBarrier2(cmdBuffer.vkCommandBuffer(), &dependencyInfo);
	}

	void prepareImageForPresentation(VkImage image, const CmdBuffer& cmdBuffer)
	{
		VkImageMemoryBarrier2 imgBarrier{ .sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 } };
		imgBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		imgBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
		imgBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_2_NONE;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imgBarrier.srcQueueFamilyIndex = cmdBuffer.queueFamily();
		imgBarrier.dstQueueFamilyIndex = cmdBuffer.queueFamily();
		imgBarrier.image = image;
		imgBarrier.subresourceRange = {
			.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
			.baseMipLevel{ 0 },
			.levelCount{ 1 },
			.baseArrayLayer{ 0 },
			.layerCount{ 1 }
		};

		VkDependencyInfo dependencyInfo{ .sType{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO } };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imgBarrier;

		vkCmdPipelineBarrier2(cmdBuffer.vkCommandBuffer(), &dependencyInfo);
	}

}