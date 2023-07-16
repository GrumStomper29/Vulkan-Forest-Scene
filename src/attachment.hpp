#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

namespace Graphics
{

	Image createColorAttachmentImage(VmaAllocator allocator, VkFormat format, VkExtent2D extent, VkSampleCountFlagBits samples);

	VkImageView createColorAttachmentImageView(VkDevice device, VkImage colorImage, VkFormat format);

	Image createDepthAttachmentImage(VmaAllocator allocator, VkExtent2D extent, VkSampleCountFlagBits samples, VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VkImageView createDepthAttachmentImageView(VkDevice device, VkImage depthImage);

	void correctColorAttachmentImageLayout(VkImage image, VkCommandBuffer commandBuffer);

	void correctDepthAttachmentImageLayout(VkImage image, VkCommandBuffer commandBuffer);

	void prepareDepthImageForSampling(VkCommandBuffer commandBuffer, VkImage image);

	VkSampler createShadowMapSampler(VkDevice device);

	void writeShadowMapSampler(VkDevice device, VkImageView imageView, VkSampler sampler, VkDescriptorSet descriptorSet);

}