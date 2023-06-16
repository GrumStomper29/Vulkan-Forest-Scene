#include "mesh.hpp"

#include "alloc.hpp"
#include "cmd_buffer.hpp"
#include "sync.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "TinyObj/tiny_obj_loader.h"

//#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
//#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Graphics
{

	void loadOBJToVertices(std::string_view path, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices)
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

		std::unordered_map<Vertex, std::uint32_t> map{};

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
					
					if (map.count(newVertex) == 0)
					{
						map[newVertex] = static_cast<std::uint32_t>(vertices.size());
						vertices.push_back(newVertex);
					}
					indices.push_back(map[newVertex]);
				}

				indexOffset += fv;
			}
		}
	}

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
			.preferredFlags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT } // Can you even do this?
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

}