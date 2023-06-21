#include "mesh.hpp"

#include "alloc.hpp"
#include "cmd_buffer.hpp"
#include "sync.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "TinyObj/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Graphics
{

	Buffer createVertexBuffer(std::vector<Vertex>& vertices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
	{
		const VkDeviceSize vertexBufferSize{ vertices.size() * sizeof(Vertex) };

		VkBufferCreateInfo stagingBufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ vertexBufferSize },
			.usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		};

		VmaAllocationCreateInfo stagingAllocCI
		{
			.flags{ VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_HOST },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};

		Buffer stagingBuffer{};
		vmaCreateBuffer(allocator, &stagingBufferCI, &stagingAllocCI, &stagingBuffer.buffer, &stagingBuffer.alloc, nullptr);

		void* data{};
		vmaMapMemory(allocator, stagingBuffer.alloc, &data);
		std::memcpy(data, vertices.data(), vertexBufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.alloc);

		vertices.clear();
		vertices.shrink_to_fit();

		VkBufferCreateInfo bufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ vertexBufferSize },
			.usage{ VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT }
		};

		VmaAllocationCreateInfo allocCI
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
		};

		Buffer buffer{};
		vmaCreateBuffer(allocator, &bufferCI, &allocCI, &buffer.buffer, &buffer.alloc, nullptr);

		vkResetCommandBuffer(commandBuffer, 0);
		beginCommandBuffer(commandBuffer, true);

		VkBufferCopy region{ .size{ vertexBufferSize } };
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &region);

		vkEndCommandBuffer(commandBuffer);

		queueSubmit(queue, commandBuffer, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);

		vkWaitForFences(device, 1, &fence, VK_TRUE, secondsToNanoseconds(600));
		vkResetFences(device, 1, &fence);

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.alloc);

		return buffer;
	}

	Buffer createIndexBuffer(std::vector<std::uint32_t>& indices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
	{
		const VkDeviceSize indexBufferSize{ indices.size() * sizeof(std::uint32_t) };

		VkBufferCreateInfo stagingBufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ indexBufferSize },
			.usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		};

		VmaAllocationCreateInfo stagingAllocCI
		{
			.flags{ VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_HOST },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};

		Buffer stagingBuffer{};
		vmaCreateBuffer(allocator, &stagingBufferCI, &stagingAllocCI, &stagingBuffer.buffer, &stagingBuffer.alloc, nullptr);

		void* data{};
		vmaMapMemory(allocator, stagingBuffer.alloc, &data);
		std::memcpy(data, indices.data(), indexBufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.alloc);

		indices.clear();
		indices.shrink_to_fit();

		VkBufferCreateInfo bufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ indexBufferSize },
			.usage{ VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT }
		};

		VmaAllocationCreateInfo allocCI
		{
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.preferredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT } // Can you even do this?
		};

		Buffer buffer{};
		vmaCreateBuffer(allocator, &bufferCI, &allocCI, &buffer.buffer, &buffer.alloc, nullptr);

		vkResetCommandBuffer(commandBuffer, 0);
		beginCommandBuffer(commandBuffer, true);

		VkBufferCopy region{ .size{ indexBufferSize } };
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &region);

		vkEndCommandBuffer(commandBuffer);

		queueSubmit(queue, commandBuffer, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);

		vkWaitForFences(device, 1, &fence, VK_TRUE, secondsToNanoseconds(600));
		vkResetFences(device, 1, &fence);

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.alloc);

		return buffer;
	}

	// Can be optimized to put the texture in better memory; same with meshes
	Image loadImage(const char* path, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
	{
		int width{};
		int height{};
		int channels{};
		stbi_uc* data{ stbi_load(path, &width, &height, &channels, STBI_rgb_alpha) };
		if (!data)
		{
			std::cerr << "failed to load texture at path: " << path << '\n';
		}

		VkDeviceSize imageSize   { static_cast<VkDeviceSize>(width * height * 4) };
		VkFormat     imageFormat { VK_FORMAT_R8G8B8A8_SRGB };

		VkBufferCreateInfo stagingBufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ imageSize },
			.usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		};
		VmaAllocationCreateInfo stagingAllocCI
		{
			.flags{ VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_HOST },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};
		Buffer stagingBuffer{};
		vmaCreateBuffer(allocator, &stagingBufferCI, &stagingAllocCI, &stagingBuffer.buffer, &stagingBuffer.alloc, nullptr);

		void* mappedData{};
		vmaMapMemory(allocator, stagingBuffer.alloc, &mappedData);
		std::memcpy(mappedData, data, imageSize);
		vmaUnmapMemory(allocator, stagingBuffer.alloc);

		stbi_image_free(data);

		VkImageCreateInfo imageCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
			.imageType{ VK_IMAGE_TYPE_2D },
			.format{ imageFormat },
			.extent{ .width{ static_cast<std::uint32_t>(width) }, .height{ static_cast<std::uint32_t>(height) }, .depth{ 1u } },
			.mipLevels{ 1 },
			.arrayLayers{ 1 },
			.samples{ VK_SAMPLE_COUNT_1_BIT },
			.tiling{ VK_IMAGE_TILING_OPTIMAL },
			.usage{ VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT },
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

		VkImageSubresourceRange subresourceRange
		{
			.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
			.baseMipLevel{ 0 },
			.levelCount{ 1 },
			.baseArrayLayer{ 0 },
			.layerCount{ 1 },
		};

		VkImageMemoryBarrier imageBarrier
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_NONE },
			.dstAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
			.oldLayout{ VK_IMAGE_LAYOUT_UNDEFINED },
			.newLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			.image{ image.image },
			.subresourceRange{ subresourceRange },
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

		VkBufferImageCopy copy
		{
			.bufferOffset{ 0 },
			.imageSubresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.imageExtent{ .width{ static_cast<std::uint32_t>(width) }, .height{ static_cast<std::uint32_t>(height) }, .depth{ 1u } },
		};
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		VkImageMemoryBarrier imageBarrier2
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
			.srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
			.dstAccessMask{ VK_ACCESS_NONE },
			.oldLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			.newLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			.image{ image.image },
			.subresourceRange{ subresourceRange },
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarrier2);

		vkEndCommandBuffer(commandBuffer);
		queueSubmit(queue, commandBuffer, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);

		vkWaitForFences(device, 1, &fence, VK_TRUE, secondsToNanoseconds(60));
		vkResetFences(device, 1, &fence);

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.alloc);

		return image;
	}

	RenderObject::RenderObject(const char* path, std::vector<Vertex>& vertices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
		: m_allocator{ allocator }
	{
		tinyobj::ObjReaderConfig readerConfig{};
		readerConfig.vertex_color = true;
		readerConfig.triangulate = true;

		tinyobj::ObjReader reader{};
		reader.ParseFromFile(static_cast<std::string>(path), readerConfig);

		if (!reader.Error().empty())
		{
			std::cerr << "error: tinyobj: " << reader.Error() << '\n';
		}

		if (!reader.Warning().empty())
		{
			std::cerr << "warning: tinyobj: " << reader.Warning() << '\n';
		}

		auto& attrib{ reader.GetAttrib() };
		auto& shapes{ reader.GetShapes() };
		auto& materials{ reader.GetMaterials() };

		//std::unordered_map<Vertex, std::uint32_t> map{};

		std::uint32_t startVert{ static_cast<std::uint32_t>(vertices.size()) };

		for (std::size_t s{ 0 }; s < shapes.size(); ++s)
		{
			std::size_t indexOffset{ 0 };
			for (std::size_t f{ 0 }; f < shapes[s].mesh.num_face_vertices.size(); ++f)
			{
				std::size_t fv{ shapes[s].mesh.num_face_vertices[f] };

				for (std::size_t v{ 0 }; v < fv; ++v)
				{
					tinyobj::index_t index{ shapes[s].mesh.indices[indexOffset + v] };

					Vertex newVertex{};
					newVertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
					newVertex.pos.y = -attrib.vertices[3 * index.vertex_index + 1];
					newVertex.pos.z = -(attrib.vertices[3 * index.vertex_index + 2]);

					if (index.normal_index >= 0)
					{
						newVertex.norm.x = attrib.normals[3 * index.normal_index + 0];
						newVertex.norm.y = attrib.normals[3 * index.normal_index + 1];
						newVertex.norm.z = attrib.normals[3 * index.normal_index + 2];
					}

					if (index.texcoord_index >= 0)
					{
						newVertex.tex.x = attrib.texcoords[2 * index.texcoord_index + 0];
						newVertex.tex.y = 1 - (attrib.texcoords[2 * index.texcoord_index + 1]);
					}

					newVertex.color.r = attrib.colors[3 * index.vertex_index + 0];
					newVertex.color.g = attrib.colors[3 * index.vertex_index + 1];
					newVertex.color.b = attrib.colors[3 * index.vertex_index + 2];

					int material{ shapes[s].mesh.material_ids[f] };
					/*
					if (map.count(newVertex) == 0)
					{
						map[newVertex] = static_cast<std::uint32_t>(vertices.size());
						vertices.push_back(newVertex);
					}
					indices.push_back(map[newVertex]);
					*/
					std::vector<Mesh>::iterator result{};
					if (meshes.size() != 0)
					{
						result = std::find_if(meshes.begin(), meshes.end(),
							[=](const Mesh& m) {
								return material == m.material;
							});
					}
					else
					{
						result = meshes.end();
					}

					if (result == meshes.end() || meshes.size() == 0)
					{
						meshes.push_back(Mesh{
							.material{ material },
							.indices{ static_cast<std::uint32_t>(vertices.size()) },
							.diffusePath{ "assets/" + materials[material].diffuse_texname },
							});
						vertices.push_back(newVertex);
					}
					else
					{
						result->indices.push_back(static_cast<std::uint32_t>(vertices.size()));
						vertices.push_back(newVertex);
					}

					//vertices.push_back(newVertex);
					//indices.push_back(static_cast<std::uint32_t>(vertices.size()) - 1);
				}

				indexOffset += fv;
			}
		}

		for (auto& m : meshes)
		{
			m.indexCount = m.indices.size();
			m.indexBuffer = createIndexBuffer(m.indices, device, allocator, queue, commandBuffer, fence);
		}
	}

	RenderObject::RenderObject(RenderObject&& r) noexcept
	{
		move(std::move(r));
	}

	RenderObject& RenderObject::operator=(RenderObject&& r) noexcept
	{
		destroy();
		move(std::move(r));
		return *this;
	}

	RenderObject::~RenderObject()
	{
		destroy();
	}

	void RenderObject::move(RenderObject&& r)
	{
		meshes = std::move(r.meshes);
		transform = r.transform;

		m_allocator = r.m_allocator;
		r.m_allocator = {};
	}

	void RenderObject::destroy()
	{
		if (m_allocator != VmaAllocator{})
		{
			for (auto& m : meshes)
			{
				vmaDestroyBuffer(m_allocator, m.indexBuffer.buffer, m.indexBuffer.alloc);
			}
		}
	}

	Texture::Texture(const char* path, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence)
		: m_device{ device },
		  m_allocator{ allocator }
	{
		m_image = loadImage(path, device, allocator, queue, commandBuffer, fence);

		VkImageViewCreateInfo imageViewCI
		{
			.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image{ m_image.image },
			.viewType{ VK_IMAGE_VIEW_TYPE_2D },
			.format{ VK_FORMAT_R8G8B8A8_SRGB },
			.subresourceRange
			{
				.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
				.baseMipLevel{ 0 },
				.levelCount{ 1 },
				.baseArrayLayer{ 0 },
				.layerCount{ 1 },
			}
		};
		vkCreateImageView(device, &imageViewCI, nullptr, &m_imageView);

		VkSamplerCreateInfo samplerCI
		{
			.sType{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO },
			.magFilter{ VK_FILTER_NEAREST },
			.minFilter{ VK_FILTER_LINEAR },
			.mipmapMode{ VK_SAMPLER_MIPMAP_MODE_NEAREST },
			.addressModeU{ VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
			.addressModeV{ VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
		};
		vkCreateSampler(device, &samplerCI, nullptr, &m_sampler);
	}

	Texture::Texture(Texture&& t)
	{
		move(std::move(t));
	}

	Texture& Texture::operator=(Texture&& t)
	{
		destroy();
		move(std::move(t));
		return *this;
	}

	Texture::~Texture()
	{
		destroy();
	}

	void Texture::move(Texture&& t)
	{
		m_image     = t.m_image;
		m_imageView = t.m_imageView;
		m_sampler   = t.m_sampler;

		m_device    = t.m_device;
		m_allocator = t.m_allocator;

		t.m_device = VK_NULL_HANDLE;
	}

	void Texture::destroy()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_device, m_sampler, nullptr);
			vkDestroyImageView(m_device, m_imageView, nullptr);
			vmaDestroyImage(m_allocator, m_image.image, m_image.alloc);
		}
	}

}