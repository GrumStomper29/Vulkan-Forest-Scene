#pragma once

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
		glm::vec3 col{};
		glm::vec2 tex{};
	};

	std::vector<std::uint32_t> loadOBJ(std::string_view path, std::vector<Vertex>& writeVertices);

}