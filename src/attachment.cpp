#include "attachment.hpp"

namespace Graphics
{

	Image createColorAttachmentImage(VmaAllocator allocator, VkFormat format, VkExtent2D extent, VkSampleCountFlagBits samples)
	{
		VkImageCreateInfo imageCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
			.imageType{ VK_IMAGE_TYPE_2D },
			.format{ format },
			.extent{ VkExtent3D{ extent.width, extent.height, 1 } },
			.mipLevels{ 1 },
			.arrayLayers{ 1 },
			.samples{ samples }, // Change for multisampling
			.tiling{ VK_IMAGE_TILING_OPTIMAL },
			.usage{ VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
			.initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
		};

		VmaAllocationCreateInfo allocCI
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		};

		Image image{};
		vmaCreateImage(allocator, &imageCI, &allocCI, &image.image, &image.alloc, nullptr);

		return image;
	}

	VkImageView createColorAttachmentImageView(VkDevice device, VkImage colorImage, VkFormat format)
	{
		VkImageViewCreateInfo viewCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image{ colorImage },
			.viewType{ VK_IMAGE_VIEW_TYPE_2D },
			.format{ format },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		VkImageView view{};
		vkCreateImageView(device, &viewCI, nullptr, &view);

		return view;
	}

	Image createDepthAttachmentImage(VmaAllocator allocator, VkExtent2D extent, VkSampleCountFlagBits samples, VkImageUsageFlags usage)
	{
		VkImageCreateInfo imageCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
			.imageType{ VK_IMAGE_TYPE_2D },
			.format{ VK_FORMAT_D32_SFLOAT },
			.extent{ VkExtent3D{ extent.width, extent.height, 1 } },
			.mipLevels{ 1 },
			.arrayLayers{ 1 },
			.samples{ samples },
			.tiling{ VK_IMAGE_TILING_OPTIMAL },
			.usage{ usage },
			.initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
		};

		VmaAllocationCreateInfo allocCI
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		};

		Image image{};
		vmaCreateImage(allocator, &imageCI, &allocCI, &image.image, &image.alloc, nullptr);

		return image;
	}

	VkImageView createDepthAttachmentImageView(VkDevice device, VkImage depthImage)
	{
		VkImageViewCreateInfo viewCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image{ depthImage },
			.viewType{ VK_IMAGE_VIEW_TYPE_2D },
			.format{ VK_FORMAT_D32_SFLOAT },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_DEPTH_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		VkImageView view{};
		vkCreateImageView(device, &viewCI, nullptr, &view);

		return view;
	}

	void correctColorAttachmentImageLayout(VkImage image, VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_NONE },
			.dstAccessMask{ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
			.newLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.image{ image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrier);
	}

	void correctDepthAttachmentImageLayout(VkImage image, VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_NONE },
			.dstAccessMask{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
			.newLayout{ VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
			.image{ image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_DEPTH_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrier);
	}

	void prepareDepthImageForSampling(VkCommandBuffer commandBuffer, VkImage image)
	{
		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT },
			.dstAccessMask{ VK_ACCESS_SHADER_READ_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
			.newLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			.image{ image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_DEPTH_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			},
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &imageBarrier);
	}

	VkSampler createShadowMapSampler(VkDevice device)
	{
		VkSamplerCreateInfo samplerCI
		{
			.sType{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO },
		};

		VkSampler sampler{};
		vkCreateSampler(device, &samplerCI, nullptr, &sampler);

		return sampler;
	}

	void writeShadowMapSampler(VkDevice device, VkImageView imageView, VkSampler sampler, VkDescriptorSet descriptorSet)
	{
		VkDescriptorImageInfo imageInfo
		{
			.sampler{ sampler },
			.imageView{ imageView },
			.imageLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		};
		VkWriteDescriptorSet write
		{
			.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
			.dstSet{ descriptorSet },
			.dstBinding{ 1 },
			.dstArrayElement{ 1000 },
			.descriptorCount{ 1 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.pImageInfo{ &imageInfo },
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

}