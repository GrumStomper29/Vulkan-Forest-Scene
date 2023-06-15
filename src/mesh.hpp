#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

#include "glm/glm.hpp"

#include <cstdint>
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
	};

	void loadOBJToVertices(std::string_view path, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices);

	Buffer createVertexBuffer(std::vector<Vertex>& vertices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

}