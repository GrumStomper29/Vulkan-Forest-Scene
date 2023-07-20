#include "texture.hpp"

#include "alloc.hpp"
#include "cmd_buffer.hpp"
#include "sync.hpp"

#include "volk/volk.h"
#include "vma/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstring>
#include <iostream>

namespace Graphics
{

	void loadImage(const char* path, stbi_uc*& data, int& width, int& height, int& channelCount)
	{
		data = stbi_load(path, &width, &height, &channelCount, STBI_rgb_alpha);
		if (!data)
		{
			std::cerr << "failed to load image at path: " << path << '\n';
		}
	}

	Image loadSkybox(const char** paths, VkDevice device, VmaAllocator allocator,
		VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
	{
		stbi_uc* datas[6]{};

		int width{};
		int height{};
		int channelCount{};

		for (int i{ 0 }; i < 6; ++i)
		{
			//// Needs cleanup
			loadImage(paths[i], datas[i], width, height, channelCount);
		}

		const VkDeviceSize layerSize{ static_cast<VkDeviceSize>(width * height * 4) };
		const VkDeviceSize imageSize{ layerSize * 6u };

		VkBufferCreateInfo stagingBufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ imageSize },
			.usage{ VK_IMAGE_USAGE_TRANSFER_SRC_BIT },
		};
		VmaAllocationCreateInfo stagingAllocCI
		{
			.flags{ VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_HOST },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};
		Buffer stagingBuffer{};
		// Needs cleanup
		vmaCreateBuffer(allocator, &stagingBufferCI, &stagingAllocCI, &stagingBuffer.buffer, &stagingBuffer.alloc, nullptr);

		void* mappedMemory{};
		vmaMapMemory(allocator, stagingBuffer.alloc, &mappedMemory);
		for (int i{ 0 }; i < 6; ++i)
		{
			void* dst{ static_cast<char*>(mappedMemory) + (layerSize * i) };
			std::memcpy(dst, datas[i], layerSize);

			stbi_image_free(datas[i]);
		}
		vmaUnmapMemory(allocator, stagingBuffer.alloc);

		VkImageCreateInfo imageCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
			.flags{ VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT },
			.imageType{ VK_IMAGE_TYPE_2D },
			.format{ VK_FORMAT_R8G8B8A8_SRGB },
			.extent{ static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 1u },
			.mipLevels{ 1 },
			.arrayLayers{ 6 },
			.samples{ VK_SAMPLE_COUNT_1_BIT },
			.tiling{ VK_IMAGE_TILING_OPTIMAL },
			.usage{ VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT },
			.initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
		};
		VmaAllocationCreateInfo allocCI
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		};
		Image image{};
		vmaCreateImage(allocator, &imageCI, &allocCI, &image.image, &image.alloc, nullptr);

		vkResetCommandBuffer(commandBuffer, 0);
		beginCommandBuffer(commandBuffer, true);

		VkImageMemoryBarrier imageMemoryBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_NONE },
			.dstAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
			.newLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			.image{ image.image },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 6 },
			},
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
			nullptr, 0,
			nullptr, 1, &imageMemoryBarrier);

		VkBufferImageCopy region
		{
			.bufferOffset{ 0 },
			.imageSubresource
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.mipLevel{ 0 },
				.baseArrayLayer{ 0 },
				.layerCount{ 6 },
			},
			.imageExtent{ static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 1u },
		};
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_NONE;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
			nullptr, 0,
			nullptr, 1, &imageMemoryBarrier);

		vkEndCommandBuffer(commandBuffer);
		queueSubmit(queue, commandBuffer, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);

		vkWaitForFences(device, 1, &fence, VK_TRUE, secondsToNanoseconds(60));
		vkResetFences(device, 1, &fence);

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.alloc);

		return image;
	}

	VkImageView createSkyboxView(VkDevice device, VkImage image)
	{
		VkImageViewCreateInfo viewCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image{ image },
			.viewType{ VK_IMAGE_VIEW_TYPE_CUBE },
			.format{ VK_FORMAT_R8G8B8A8_SRGB },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 6 },
			},
		};

		VkImageView view{};
		vkCreateImageView(device, &viewCI, nullptr, &view);

		return view;
	}

	VkSampler createSkyboxSampler(VkDevice device)
	{
		VkSamplerCreateInfo samplerCI
		{
			.sType{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO },
			.magFilter{ VK_FILTER_LINEAR },
			.minFilter{ VK_FILTER_LINEAR },
		};

		VkSampler sampler{};
		vkCreateSampler(device, &samplerCI, nullptr, &sampler);

		return sampler;
	}

}