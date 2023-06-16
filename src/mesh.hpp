#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace Graphics
{

	struct Vertex
	{
		glm::vec3 pos{};
		glm::vec3 norm{};
		glm::vec3 color{};
		glm::vec2 tex{};

		bool operator==(const Vertex& v) const
		{
			return pos == v.pos && norm == v.norm && color == v.color && tex == v.tex;
		}
	};

	void loadOBJToVertices(std::string_view path, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices);

	Buffer createVertexBuffer(std::vector<Vertex>& vertices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

	Buffer createIndexBuffer(std::vector<std::uint32_t>& indices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

}

template<>
struct std::hash<Graphics::Vertex>
{
	size_t operator()(const Graphics::Vertex& v) const noexcept
	{
		size_t h1{ hash<glm::vec3>{}(v.pos) };
		size_t h2{ hash<glm::vec3>{}(v.norm) };
		size_t h3{ hash<glm::vec3>{}(v.color) };
		size_t h4{ hash<glm::vec2>{}(v.tex) };

		constexpr std::uint64_t multiplier{ 6364136223846793005 };
		constexpr std::uint64_t increment{ 1442695040888963407 };

		size_t finalHash{ h1 };
		finalHash = finalHash ^ (h2 * multiplier + increment);
		finalHash = finalHash ^ (h3 * multiplier + increment);
		finalHash = finalHash ^ (h4 * multiplier + increment);

		return finalHash;
	}
};